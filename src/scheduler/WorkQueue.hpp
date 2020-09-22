#pragma once

#include <queue>
#include <memory>
#include <atomic>

#include "latch/lock.hpp"
#include "concurrent/fiber.hpp"
#include "macro.hpp"
#include "logs.hpp"

#define NO_LIMIT                      ((size_t)-1)
#define FETCH_ALL                     NO_LIMIT
namespace zhuyh {


template<size_t LEVEL = 3>
class WorkQueue {
public:
    typedef std::shared_ptr<WorkQueue> ptr;
    enum Priority : char{
        LOW = 0,
        NORMAL =1,
        EMERG = 2,
        NONE = 3,
    };

    WorkQueue(size_t limit = NO_LIMIT)
    :  m_limit(limit) {
    }

    ~WorkQueue() {
        for (size_t i = 0; i < LEVEL; i++) {
            ASSERT2(m_queue[i].empty(), getPriorityName((Priority)i)+ " queue is not empty now!");
        }
    }
    static std::string getPriorityName(Priority prio) {
#define XX(LEVEL)                       \
        if (LEVEL == prio) {            \
            return #LEVEL;               \
        }
        XX(LOW);
        XX(NORMAL);
        XX(EMERG);
#undef XX

        return "NONE";
    }

    size_t sizeStrong(Priority prio = Priority::NORMAL) const {
        LockGuard lg(m_mx);
        return sizeRelax(prio);
    }

    size_t sizeRelax(Priority prio = Priority::NORMAL) const {
        if (prio >= LEVEL) {
            return NO_LIMIT;
        }
        return m_queue[prio].size();
    }

    bool emptyStrong() const {
        LockGuard lg(m_mx);
        return emptyRelax();
    }

    bool emptyRelax() const {
        for (size_t i = 0; i < LEVEL; i++) {
            if (!m_queue[i].empty()) {
                return false;
            }
        }
        return true;
    }

    /**
     * @brief add a fiber with a specific priority, default Priority::NORMAL
     */
    bool addFiber(Fiber::ptr fiber, Priority prio = Priority::NORMAL) {
        if (prio >= Priority::NONE || fiber == nullptr) {
            return false;
        }
        LockGuard lg(m_mx);
        if (isLimited() && ((m_queue[prio].size() + 1) > m_limit)) {
            return false;
        }
        m_queue[prio].push(fiber);
        return true;
    }
    /**
     * @brief add many fibers and only lock once. add as many fiber as possible.
     * @param[in] fibers the fibers about to add the the workqueue
     * @param[out] left no space left for those fibers
     * @param[in] prio the priority of those fibers
     * @return return true for success and false for failure
     */
    size_t addFibers(const std::list<Fiber::ptr> &fibers, 
                   std::list<Fiber::ptr> &left,
                   Priority prio = Priority::NORMAL) {
        size_t nFiber = fibers.size();
        size_t res = 0;
        typename std::list<Fiber::ptr>::const_iterator iter = fibers.begin();

        if (prio >= Priority::NONE || fibers.empty()) {
            return 0;
        }

        {
            LockGuard lg(m_mx);
            if (isLimited()) {
                nFiber = std::min(nFiber, m_limit - m_queue[prio].size());
            }
            while (iter != fibers.end()) {
                if (*iter != nullptr) {
                    m_queue[prio].push(*iter);
                    ++res;
                    if (--nFiber == 0) {
                        break;
                    }
                }
                ++iter;
            }
        }

        while (iter != fibers.end()) {
            if (*iter != nullptr) {
                left.push_back(*iter);
            }
            ++iter;
        }

        return res;
    }
    /**
     * @brief fetch a fiber
     * @param[out] fiber fiber fetched will be stored here
     * @return return the priority of fiber Priority::NONE for no fiber
     */
    Priority fetchFiber(Fiber::ptr &fiber) {
        LockGuard lg(m_mx);
        for (size_t i = 0; i < LEVEL; i++) {
            if (!m_queue[i].empty()) {
                fiber = m_queue[i].front();
                m_queue[i].pop();
                return (Priority)i;
            }
        }

        return Priority::NONE;
    }
    /**
     * @brief fetch a fiber with a certain priority. This is NOT recommand to use
     * @param[out] fiber fiber fetched will be stored here
     * @return return true for success and false for failure
     */
    bool fetchFiberPrio(Fiber::ptr &fiber, Priority prio = Priority::NORMAL) {
        if (prio >= Priority::NONE) {
            return false;
        }
        LockGuard lg(m_mx);
        if (!m_queue[prio].empty()) {
            fiber = m_queue[prio].front();
            m_queue[prio].pop();
            return true;
        }

        return false;
    }

    /**
     * @brief fetch a certain number of fibers with a certain priority. This is for scheduling and NOT recommand to use
     * @param[out] fiber fiber fetched will be stored here
     * @return return number of fiber fetched
     */
    size_t fetchFibers(size_t nr, std::list<Fiber::ptr> &fibers, Priority prio = Priority::NORMAL) {
        size_t res = 0;

        if (prio >= Priority::NONE) {
            return 0;
        }
        LockGuard lg(m_mx);
        while (!m_queue[prio].empty() && nr--) {
            fibers.push_back(m_queue[prio].front());
            m_queue[prio].pop();
            res++;
        }

        return res;
    }

     /**
     * @brief fetch all of the fibers with a certain priority. This is for scheduling and NOT recommand to use
     * @param[out] fiber fiber fetched will be stored here
     * @return return number of fiber fetched
     */
    size_t fetchAll(std::list<Fiber::ptr> &fibers, Priority prio = Priority::NORMAL) {
        return fetchAll(FETCH_ALL, fibers, prio);
    }

    /**
     * @brief show whether the size of workqueue is limited
     */
    bool isLimited() const {
        return m_limit != NO_LIMIT;
    }

    /**
     * @brief the limit size
     */
    size_t getLimit() const {
        return m_limit;
    }
private:
    size_t m_limit;
    std::queue<Fiber::ptr> m_queue[LEVEL];
    mutable Mutex m_mx;
};

} // end of namespace zhuyh