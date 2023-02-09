cmake_minimum_required(VERSION 3.24)
project(internet_radio)

set(CMAKE_CXX_STANDARD 23)

add_executable(internet_radio main.cpp AudioStreamer.cpp AudioStreamer.h ClientManager.cpp ClientManager.h Client.cpp Client.h Request.cpp Request.h)
