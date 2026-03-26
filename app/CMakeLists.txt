cmake_minimum_required(VERSION 3.20)
project(codex-installer LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(SDL2_IMAGE REQUIRED SDL2_image)
pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)
pkg_check_modules(SDL2_MIXER REQUIRED SDL2_mixer)

add_executable(codex-installer
    main.cpp
    VclRenderer.cpp
    InstallerWindow.cpp
)

target_include_directories(codex-installer PRIVATE
    ${SDL2_INCLUDE_DIRS}
    ${SDL2_IMAGE_INCLUDE_DIRS}
    ${SDL2_TTF_INCLUDE_DIRS}
    ${SDL2_MIXER_INCLUDE_DIRS}
)

target_link_libraries(codex-installer PRIVATE
    ${SDL2_LIBRARIES}
    ${SDL2_IMAGE_LIBRARIES}
    ${SDL2_TTF_LIBRARIES}
    ${SDL2_MIXER_LIBRARIES}
)
