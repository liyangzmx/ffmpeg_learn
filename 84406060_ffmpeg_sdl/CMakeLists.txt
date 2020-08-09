cmake_minimum_required(VERSION 3.0.0)
project(ffmpeg_sdl_learn VERSION 0.1.0)

include(FindPkgConfig)

include(CTest)
enable_testing()

find_package(PkgConfig REQUIRED)

pkg_check_modules(LIBAV REQUIRED IMPORTED_TARGET
    libavdevice
    libavfilter
    libavformat
    libavcodec
    libswresample
    libswscale
    libavutil
)

pkg_check_modules(LIBSDL REQUIRED IMPORTED_TARGET
    sdl2
    SDL_image
)

add_executable(ffmpeg_sdl_learn main.cpp)

target_link_libraries(
    ffmpeg_sdl_learn
    PkgConfig::LIBAV
    PkgConfig::LIBSDL
)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
