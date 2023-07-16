#include "logUtil.hpp"
#include "fiber.hpp"
#include "log.hpp"
#include "stackTrait.hpp"
#include "macro.hpp"
#include <sys/mman.h>

namespace zhuyh {
static Logger::ptr s_syslog = GET_LOGGER("system");
static std::atomic<uint32_t> __fiber_id {0};

//默认协程栈大小
static ConfigVar<size_t>::ptr __fiber_stack_size = Config::lookUp<size_t>("fiber.stack_size",64*1024,"__stack_size");
class MMapcAlloc {
public:
    static char* alloc(size_t size) {
        return (char*)mmap(0,size,PROT_READ | PROT_WRITE,
			    MAP_PRIVATE | MAP_ANON,-1,0);
    }
    static void dealloc(void* ptr,size_t size)
    {
      munmap(ptr,size);
    }
};

class MallocAlloc {
  public:
    static char* alloc(size_t size) {
      return (char*)malloc(size);
    }
    static void dealloc(void* ptr,size_t size) {
      free(ptr);
    }
};
  
Fiber*& Fiber::_this_fiber() {
    static thread_local Fiber* __this_fiber = nullptr;
    return __this_fiber;
}
  
Fiber::ptr& Fiber::_main_fiber() {
    static thread_local Fiber::ptr _threadFiber = nullptr;
    return _threadFiber;
}
  
using Allocator = MMapcAlloc;
  //主协程
Fiber::Fiber()
    :_state(EXEC),
     isMain(true) {
    setThis(this);
    _main_fiber();
    _this_fiber();
}
  
Fiber::Fiber(CbType cb,size_t size)
    :_cb(cb),
     _fid(++__fiber_id),
     _state(INIT) {
    _stackSize = size ? size : __fiber_stack_size->getVar();
    //多申请pages_protect页
    size_t pages = StackTrait::getProtectStackPageSize();
    _stackSize += getpagesize()*pages;
    _stack = Allocator::alloc(_stackSize);
    ASSERT2(_stack != nullptr,"Out of Memory");
    //std::cout<<_stackSize<<std::endl;
    _ctx = make_fcontext((char*)_stack+_stackSize,_stackSize,Fiber::run);
    if (StackTrait::protectStack(_stack,_stackSize,pages)) {
	    _pagesProtect = pages;
    }
}

Fiber::~Fiber() {
    //LOG_ROOT_INFO() << "Fiber ID : "<<_fid<<" Destroyed!";
    if(_stack) {
        ASSERT2(_state == TERM || _state == INIT
                || _state == EXCEPT,getState(_state));
        //使用了栈保护且不是主协程
        if(_pagesProtect && !isMain)
            StackTrait::unprotectStack(_stack,_pagesProtect);
        Allocator::dealloc(_stack,_stackSize);
        _stack = nullptr;
    } else {
        if(isMain)
        _state = EXEC;
        ASSERT(!(_cb));
        ASSERT(_state == EXEC);

        Fiber* cur = _this_fiber();
        if(cur == this) {
            setThis(nullptr);
        }
    }
}
  
//状态必须是INIT/TERM
void Fiber::reset(CbType cb) {
    ASSERT2(_state == INIT || _state == TERM
	         || _state == EXCEPT,"Illegal State");
    ASSERT(_stack);
    _cb = cb;
    _ctx = make_fcontext((char*)_stack+_stackSize,_stackSize,&Fiber::run);
    _state = INIT;
}

void Fiber::swapIn() {
    setThis(this);
    ASSERT(_state != EXEC);
    _state = EXEC;
    if(_main_fiber() == nullptr)
      ASSERT(false);
    jump_fcontext(&(_main_fiber()->_ctx),_ctx,0);
}

void Fiber::swapOut() {
    //由于是星型调度,因此主协程只会将其他协程swapIn,而不会调用swapOut
    if(getThis() == _main_fiber())
        return;
    setThis(_main_fiber().get());
    if(jump_fcontext(&_ctx,_main_fiber()->_ctx,0)) {
	    ASSERT2(false,"jump_fcontext error");
    }
}

//子协程间切换,不暴露给用户
void Fiber::swapTo(Fiber::ptr fiber) {
    if (fiber == getThis())
        return;
    setThis(fiber.get());
    if (jump_fcontext(&_ctx,fiber->_ctx,0)) {
	    ASSERT2(false,"jump_fcontext error");
    }
}

void Fiber::setThis(Fiber* fiber) {
    _this_fiber() = fiber;
}

  //获取当前协程,没有则返回主协程
Fiber::ptr Fiber::getThis() {
    if(_this_fiber())
      return _this_fiber()->shared_from_this();
    Fiber::ptr mainFiber(new Fiber);
    ASSERT(_this_fiber() == mainFiber.get());
    _main_fiber() = mainFiber;
    return _this_fiber()->shared_from_this();
}

void Fiber::YieldToReady() {
    Fiber::ptr cur = getThis();
    cur->_state = READY;
    cur->swapOut();
}
  
void Fiber::YieldToHold() {
    Fiber::ptr cur = getThis();
    if(cur == _main_fiber() ) return;
    ASSERT(cur!=nullptr);
    setThis(_main_fiber().get());
    if(jump_fcontext(&(cur->_ctx),_main_fiber()->_ctx,0)) {
	    ASSERT2(false,"jump_fcontext error");
    }
}

//不可以使用日志,会产生递归!!!!!!!!!!!!!!
uint32_t Fiber::getFid() {
    if(_this_fiber()) {
        return _this_fiber()->_fid;
    }
    return 0;
}

void Fiber::run(int) {
    Fiber::ptr cur = getThis();
    ASSERT(cur);
    try {
        cur->_cb();
        cur->_cb = nullptr;
        cur->_state = TERM;
    } catch(std::exception& e) {
        cur->_state = EXCEPT;
        LOG_ERROR(s_syslog) << "Fiber Except : "<< e.what();
    } catch(...) {
        cur->_state = EXCEPT;
        LOG_ERROR(s_syslog) << "Fiber Except : UNKNOWN";
    }
    //获取裸指针,防止智能指针一直停留在协程栈上导致无法析构
    auto raw_ptr = cur.get();
    //使得引用减一,让协程在主协程析构
    cur.reset();
    //LOG_ROOT_INFO() << "mainFunc ID:"<<getFid();
    raw_ptr->swapOut();
}

};
