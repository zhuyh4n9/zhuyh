/* 
 *TODO: avoid using Task, use fiber directly in Scheduler, Reactor and Processor
 */

#pragma once

#include <memory>
#include "concurrent/fiber.hpp"

namespace zhuyh
{
struct Task
  {    
    typedef std::shared_ptr<Task> ptr;
    Fiber::ptr fiber = nullptr;
    Fiber::CbType cb = nullptr;
    bool stealable = true;
    bool finTag = false;
    Task(Fiber::ptr f)
      :fiber(f)
    {
    }
    Task(Fiber::ptr* f)
    {
      fiber.swap(*f);
    }
    Task(Fiber::CbType c)
      :cb(c)
    {
    }
    Task(Fiber::CbType* c)
    {
      cb.swap(*c);
    }
    static std::atomic<int> id;
    Task(){}
    ~Task()
    {
      // LOG_ROOT_INFO() << "Destroying id : "<<_id;
    }
  };
}
