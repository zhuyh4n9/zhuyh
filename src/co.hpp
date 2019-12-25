#pragma once

#include "scheduler/Scheduler.hpp"
#include "scheduler/Processer.hpp"
#include "scheduler/IOManager.hpp"
#include "concurrent/fiber.hpp"


namespace zhuyh
{
  void Co(Fiber::CbType cb);
};
