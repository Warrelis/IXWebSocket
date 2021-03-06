#
# Author: Benjamin Sergeant
# Copyright (c) 2018 Machine Zone, Inc. All rights reserved.
#
cmake_minimum_required (VERSION 3.4.1)
project (ixwebsocket_unittest)

set (CMAKE_CXX_STANDARD 14)

if (NOT WIN32)
  set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/../third_party/sanitizers-cmake/cmake" ${CMAKE_MODULE_PATH})
  find_package(Sanitizers)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread")
  set(CMAKE_LD_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread")
  option(USE_TLS "Add TLS support" ON)
endif()

add_subdirectory(${PROJECT_SOURCE_DIR}/.. ixwebsocket)

include_directories(
  ${PROJECT_SOURCE_DIR}/Catch2/single_include
  ../third_party/msgpack11
  ../third_party/spdlog/include
  ../ws
)

# Shared sources
set (SOURCES 
  test_runner.cpp
  IXTest.cpp
  ../third_party/msgpack11/msgpack11.cpp
  ../ws/ixcore/utils/IXCoreLogger.cpp

  IXSocketTest.cpp
  IXSocketConnectTest.cpp
  IXWebSocketServerTest.cpp
  IXWebSocketTestConnectionDisconnection.cpp
  IXUrlParserTest.cpp
  IXWebSocketServerTest.cpp
  IXHttpClientTest.cpp
)

# Some unittest don't work on windows yet
if (UNIX)
  list(APPEND SOURCES
    IXDNSLookupTest.cpp
    cmd_websocket_chat.cpp
    IXWebSocketCloseTest.cpp
  )
endif()

# Some unittest fail for dubious reason on Ubuntu Xenial with TSAN
if (MAC OR WIN32)
  list(APPEND SOURCES
    IXWebSocketMessageQTest.cpp
  )
endif()

# Ping test fails intermittently, disabling them for now
# IXWebSocketPingTest.cpp
# IXWebSocketPingTimeoutTest.cpp

# Disable tests for now that are failing or not reliable

add_executable(ixwebsocket_unittest ${SOURCES})

if (NOT WIN32)
  add_sanitizers(ixwebsocket_unittest)
endif()

if (APPLE AND USE_TLS)
    target_link_libraries(ixwebsocket_unittest "-framework foundation" "-framework security")
endif()

target_link_libraries(ixwebsocket_unittest ixwebsocket)
install(TARGETS ixwebsocket_unittest DESTINATION bin)
