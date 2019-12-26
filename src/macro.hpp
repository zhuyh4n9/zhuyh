#pragma once

#include <assert.h>
#include <cstring>
#include "util.hpp"
#include "logUtil.hpp"
#include "concurrent/fiber.hpp"

#ifndef co_yield
#define co_yield zhuyh::Fiber::YieldToReady()
#define co_yield_to_hold zhuyh::Fiber::YieldToHold()
#endif

#ifndef co
#define co zhuyh::__co()=
#endif

#define ASSERT(x)						\
  if(!(x))							\
    {								\
      LOG_ROOT_ERROR()<<"ASSERTION: "<<#x			\
		  <<"\n";					\
      assert(x);						\
    }

#define ASSERT2(x,w)						\
   if(!(x))							\
     {								\
       LOG_ROOT_ERROR()<<"ASSERTION: " #x			\
		       <<"\n"<< w  <<"\n";			\
	assert(x);						\
    }
