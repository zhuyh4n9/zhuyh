cmake_minimum_required(VERSION 2.8)
ENABLE_LANGUAGE(ASM)
project(zhuyh)
set(CMAKE_CXX_FLAGS
  "$ENV{CXXFLAGS} -rdynamic -O2 -g  -std=c++1z -Wall -Werror -Wno-unused-function -fstack-protector-all")

set(LIB_SRC
  src/log.cpp
  src/logUtil.cpp
  src/config.cpp
  src/concurrent/Thread.cpp
  src/latch/lock.cpp
  src/util.cpp
  src/LogThread.cpp
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
  src/netio/Hook.cpp
  src/netio/FdManager.cpp
  src/socket/Address.cpp
  src/socket/Socket.cpp
  src/socket/TcpServer.cpp
  src/bytearray/ByteArray.cpp
  src/http/Http.cpp
  src/http/http11_parser.rl.cpp
  src/http/httpclient_parser.rl.cpp
  src/http/HttpParser.cpp
  src/http/HttpSession.cpp
  src/http/HttpConnection.cpp
  src/http/HttpServer.cpp
  src/http/Servlet.cpp
  src/stream/Stream.cpp
  src/stream/SocketStream.cpp
  src/db/mysql.cpp)

set(LIBS 
    zhuyh  
    dl
    pthread
    yaml-cpp
    mysqlclient)
SET(SERIALIZE_LIBS
    src/Serialize/reflect.cpp)

SET(PROXY_LIBS
    src/httpProxy/LogInServlet.cpp
    src/httpProxy/main.cpp)	
add_library(zhuyh SHARED ${LIB_SRC})
add_library(libzhuyh STATIC  ${LIB_SRC})

add_executable(Serialize src/Serialize/main.cpp ${SERIALIZE_LIBS})
target_link_libraries(Serialize tinyxml2)

add_executable(Proxy ${PROXY_LIBS})
target_link_libraries(Proxy ${LIBS})
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

#Add_Executable(test_processer test/test_processer.cpp)
#target_link_libraries(test_processer zhuyh  ${LIBS})

#add_executable(test_iomgr test/test_iomgr.cpp )
#target_link_libraries(test_iomgr zhuyh  ${LIBS})

#add_executable(test_co test/test_co.cpp)
#target_link_libraries(test_co zhuyh  ${LIBS})

#add_executable(test_timer test/test_timer.cpp)
#target_link_libraries(test_timer zhuyh ${LIBS})

add_executable(test_hook test/test_hook.cpp)
target_link_libraries(test_hook zhuyh ${LIBS})

add_executable(test_client test/test_client.cpp)
target_link_libraries(test_client zhuyh ${LIBS})

add_executable(test_http test/test_http.cpp)
target_link_libraries(test_http zhuyh ${LIBS})

add_executable(test_address test/test_address.cpp)
target_link_libraries(test_address zhuyh ${LIBS})

add_executable(test_pipe test/test_pipe.cpp)
target_link_libraries(test_pipe zhuyh ${LIBS})

add_executable(test_socket test/test_socket.cpp)
target_link_libraries(test_socket zhuyh ${LIBS})

add_executable(test_bytearray test/test_bytearray.cpp)
target_link_libraries(test_bytearray zhuyh ${LIBS})


add_executable(http_test test/http_test.cpp)
target_link_libraries(http_test zhuyh ${LIBS})

add_executable(test_httpparser test/test_httpparser.cpp)
target_link_libraries(test_httpparser zhuyh ${LIBS})

add_executable(test_tcpserver test/test_tcpserver.cpp)
target_link_libraries(test_tcpserver zhuyh ${LIBS})

add_executable(echo_server example/echo_server.cpp)
target_link_libraries(echo_server zhuyh ${LIBS})

add_executable(test_httpServer test/test_httpServer.cpp)
target_link_libraries(test_httpServer zhuyh ${LIBS})

add_executable(test_httpConnection test/test_httpConnection.cpp)
target_link_libraries(test_httpConnection zhuyh ${LIBS})

add_executable(test_mysql test/test_mysql.cpp)
target_link_libraries(test_mysql zhuyh ${LIBS})

add_executable(test_stmt test/test_stmt.cpp)
target_link_libraries(test_stmt ${LIBS})


set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

