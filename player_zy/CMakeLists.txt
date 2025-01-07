cmake_minimum_required(VERSION 3.1.1)

project(zy_player_lib VERSION 1.0.0 DESCRIPTION "ZY Player Library")
set(CMAKE_BUILD_TYPE Release)

set(LINK_LIBRARIES ${MK_LINK_LIBRARIES})


set(PLAYER_NAME "zy_player_lib")

find_package(PkgConfig QUIET)
# 查找 SDL2 是否安装
if(PKG_CONFIG_FOUND)
    pkg_check_modules(SDL2 QUIET IMPORTED_TARGET sdl2)
    if(SDL2_FOUND)
        list(APPEND LINK_LIBRARIES PkgConfig::SDL2)
        message(STATUS "found library: ${SDL2_LIBRARIES}")
    endif()
endif()

if(NOT SDL2_FOUND)
    find_package(SDL2 QUIET)
    if(SDL2_FOUND)
        include_directories(SYSTEM ${SDL2_INCLUDE_DIR})
        list(APPEND LINK_LIBRARIES ${SDL2_LIBRARY})
        message(STATUS "found library: ${SDL2_LIBRARY}")
    endif()
endif()

# 如果 ffmpeg/libavcodec ffmpeg/libavcodec SDL 都安装了则编译播放器
if(NOT SDL2_FOUND)
    message(WARNING "${PLAYER_NAME} disabled, please install sdl2 ffmpeg/libavcodec ffmpeg/libavutil ffmpeg/libswresample")
    return()
endif()

message(STATUS "${PLAYER_NAME} enabled")


# TODO: 统一参数?
#if(MSVC)
#    set_target_properties(${PLAYER_NAME} PROPERTIES LINK_FLAGS "/SAFESEH:NO /SUBSYSTEM:WINDOWS")
#endif()
#
#if(CMAKE_SYSTEM_NAME MATCHES "Linux")
#    target_link_libraries(${PLAYER_NAME} -Wl,--start-group ${LINK_LIBRARIES} -Wl,--end-group)
#else()
#    target_link_libraries(${PLAYER_NAME} ${LINK_LIBRARIES})
#endif()
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(../3rdpart/ZLToolKit)

add_library(zy_player_lib SHARED
    ./source/zy_player.cpp
)


target_include_directories(zy_player_lib PRIVATE ../3rdpart/ZLToolKit)

set_target_properties(zy_player_lib PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(zy_player_lib PROPERTIES PUBLIC_HEADER ./include/zy_player.h)
set_target_properties(zy_player_lib PROPERTIES PUBLIC_HEADER include/display_helper.h)

# TODO: 统一参数?
if(MSVC)
    set_target_properties(${PLAYER_NAME} PROPERTIES LINK_FLAGS "/SAFESEH:NO /SUBSYSTEM:WINDOWS")
endif()

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    target_link_libraries(${PLAYER_NAME} -Wl,--start-group ${LINK_LIBRARIES} -Wl,--end-group)
else()
    target_link_libraries(${PLAYER_NAME} ${LINK_LIBRARIES})
endif()

