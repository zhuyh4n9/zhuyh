cmake_minimum_required(VERSION 2.8)
ENABLE_LANGUAGE(ASM)
project(zhuyh)
set(CMAKE_CXX_FLAGS
  "$ENV{CXXFLAGS} -rdynamic -O2 -g  -std=c++11 -Wall -Werror -Wno-unused-function")

set(LIB_SRC
  src/log.cpp
  src/logUtil.cpp
  src/config.cpp
  src/concurrent/Thread.cpp
  src/latch/lock.cpp
  src/util.cpp
  src/concurrent/fiber.cpp
  src/asm/make_x86_64_sysv_elf_gas.S
  src/asm/jump_x86_64_sysv_elf_gas.S
  src/concurrent/stackTrait.cpp
  src/scheduler/Processer.cpp
  src/scheduler/IOManager.cpp
  src/scheduler/Scheduler.cpp
  src/co.cpp
  src/scheduler/TimerManager.cpp
  src/latch/coLock.cpp
  src/NetIO/Hook.cpp)

set(LIBS yaml-cpp pthread dl)
add_library(zhuyh SHARED ${LIB_SRC})
add_library(libzhuyh STATIC  ${LIB_SRC})

#add_executable(logtest test/logtest.cpp)
#target_link_libraries(logtest zhuyh ${LIBS})

#add_executable(threadtest test/threadtest.cpp)
#target_link_libraries(threadtest zhuyh ${LIBS})

#add_executable(performance test/log_performance.cpp)
#target_link_libraries(performance zhuyh ${LIBS})

#add_executable(performance_compare test/performance_compare.cpp)
#target_link_libraries(performance_compare zhuyh ${LIBS})

#add_executable(test_backtrace test/test_backtrace.cpp)
#target_link_libraries(test_backtrace zhuyh ${LIBS})

#add_executable(test_fiber test/test_fiber.cpp)
#target_link_libraries(test_fiber zhuyh  ${LIBS})

#add_executable(test_queue test/test_queue.cpp)
#target_link_libraries(test_queue zhuyh  ${LIBS})

add_executable(test_processer test/test_processer.cpp)
target_link_libraries(test_processer zhuyh  ${LIBS})

add_executable(test_iomgr test/test_iomgr.cpp )
target_link_libraries(test_iomgr zhuyh  ${LIBS})

add_executable(test_co test/test_co.cpp)
target_link_libraries(test_co zhuyh  ${LIBS})

add_executable(test_timer test/test_timer.cpp)
target_link_libraries(test_timer zhuyh ${LIBS})

set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

