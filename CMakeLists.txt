cmake_minimum_required(VERSION 3.24)
project(internet_radio)

if( CMAKE_BINARY_DIR STREQUAL CMAKE_SOURCE_DIR )
    message( FATAL_ERROR "Please select another Build Directory" )
endif()
if( CMAKE_SOURCE_DIR MATCHES " " )
    message( "Your Source Directory contains spaces. If you experience problems when compiling, this can be the cause." )
endif()
if( CMAKE_BINARY_DIR MATCHES " " )
    message( "Your Build Directory contains spaces. If you experience problems when compiling, this can be the cause." )
endif()

include_directories(external/irrKlang_1_6_0/include/.)

link_directories(external/irrKlang_1_6_0/lib/)

set(ALL_LIBS libIrrKlang.so)

set(CMAKE_CXX_STANDARD 23)

add_executable(internet_radio_client mainClient.cpp MusicPlayer.cpp MusicPlayer.h Packet.cpp Packet.h)

target_link_libraries(internet_radio_client  ${ALL_LIBS})

if (NOT ${CMAKE_GENERATOR} MATCHES "Xcode" )
    add_custom_command(
            TARGET internet_radio_client POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/internet_radio_client${CMAKE_EXECUTABLE_SUFFIX}" "${CMAKE_CURRENT_SOURCE_DIR}/bin"
    )
elseif (${CMAKE_GENERATOR} MATCHES "Xcode" )

endif (NOT ${CMAKE_GENERATOR} MATCHES "Xcode" )

add_executable(internet_radio_server mainServer.cpp AudioStreamer.cpp AudioStreamer.h ClientManager.cpp ClientManager.h Client.cpp Client.h Packet.cpp Packet.h)

if (NOT ${CMAKE_GENERATOR} MATCHES "Xcode" )
    add_custom_command(
            TARGET internet_radio_server POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/internet_radio_server${CMAKE_EXECUTABLE_SUFFIX}" "${CMAKE_CURRENT_SOURCE_DIR}/bin"
    )
elseif (${CMAKE_GENERATOR} MATCHES "Xcode" )

endif (NOT ${CMAKE_GENERATOR} MATCHES "Xcode" )
