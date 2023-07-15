#pragma once

#include <assert.h>
#include <cstring>
#include "util.hpp"
#include "concurrent/fiber.hpp"

#ifndef yield_co
#define yield_co zhuyh::Fiber::YieldToReady()
#define yield_co_to_hold zhuyh::Fiber::YieldToSwitch()
#endif

#ifndef co
#define co zhuyh::__co()=
#endif

#define ASSERT(x)					                  	\
  if(!(x))					                      		\
    {								                          \
      LOG_ROOT_FATAL()<<"ASSERTION: "<<#x			\
		  <<"\n";				                        	\
    }

#define ASSERT2(x,w)					                \
   if(!(x))							                      \
     {								                        \
       LOG_ROOT_FATAL()<<"ASSERTION: " #x			\
		       <<"\n"<< (w)  <<"\n";			        \
    }
