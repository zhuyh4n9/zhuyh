cmake_minimum_required(VERSION 2.8)
ENABLE_LANGUAGE(ASM)
project(zhuyh)
set(CMAKE_CXX_FLAGS
  "$ENV{CXXFLAGS} -rdynamic -O2 -g  -std=c++2a -Wall -Werror -Wno-unused-function -fstack-protector-all -Lout/yaml")

INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/src/config
    out/yaml/include)

set(LIB_SRC
  src/log.cpp
  src/logUtil.cpp
  src/config/Config.cpp
  src/concurrent/Thread.cpp
  src/latch/lock.cpp
  src/util.cpp
  src/LogThread.cpp
  src/concurrent/fiber.cpp
  src/asm/make_x86_64_sysv_elf_gas.S
  src/asm/jump_x86_64_sysv_elf_gas.S
  src/concurrent/stackTrait.cpp
  src/scheduler/Processer.cpp
  src/scheduler/Reactor.cpp
  src/scheduler/Scheduler.cpp
  src/co.cpp
  src/scheduler/TimerManager.cpp
  src/latch/coLock.cpp
  src/netio/Hook.cpp
  src/netio/FdManager.cpp
  src/socket/Address.cpp
  src/socket/Socket.cpp
  src/socket/TcpServer.cpp
  src/bytearray/ByteArray.cpp
  src/stream/Stream.cpp
  src/stream/SocketStream.cpp
  src/crypto/CryptoUtil.cpp
)

find_library (yamlcpp yaml-cpp  ./out/lib/)
message("project to be compiled: " ${yamlcpp})

set(LIBS 
    zhuyh  
    dl
    pthread
    ${yamlcpp}
)


add_library(zhuyh SHARED ${LIB_SRC})
add_library(libzhuyh STATIC  ${LIB_SRC})

add_executable(test_co test/test_co.cpp)
target_link_libraries(test_co ${LIBS})

set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/out/lib)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/out/bin)

