cmake_minimum_required(VERSION 3.0)

project(DualViewBridge)

if(WIN32)
  
else(WIN32)

  # Has to be on one line or else ';'s will be included
  # C++14
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wall -Wno-unused-function -Wno-unused-variable -Wno-unknown-pragmas -Wl,--no-undefined -Wl,--no-allow-shlib-undefined -Wno-pragma-once-outside-header")
  
endif(WIN32)

add_executable(dualview_bridge Main.cpp json.hpp)

install(TARGETS dualview_bridge DESTINATION "/usr/local/bin")

install(FILES dualview_bridge.json PERMISSIONS WORLD_READ OWNER_WRITE OWNER_READ
  DESTINATION "/usr/lib64/mozilla/native-messaging-hosts/")
