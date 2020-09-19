#pragma once

#include <assert.h>
#include <cstring>
#include "util.hpp"
#include "concurrent/fiber.hpp"

#ifndef co_yield
#define co_yield zhuyh::Fiber::YieldToReady()
#define co_yield_to_hold zhuyh::Fiber::YieldToSwitch()
#endif

#ifndef co
#define co zhuyh::__co()=
#endif

#define ASSERT(x)						\
  if(!(x))							\
    {								\
      LOG_ROOT_FATAL()<<"ASSERTION: "<<#x			\
		  <<"\n";					\
    }

#define ASSERT2(x,w)						\
   if(!(x))							\
     {								\
       LOG_ROOT_FATAL()<<"ASSERTION: " #x			\
		       <<"\n"<< (w)  <<"\n";			\
    }
