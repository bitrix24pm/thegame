cmake_minimum_required(VERSION 3.16)
project(architect_of_the_void CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Минимальный размер бинарника: -Os, без RTTI/исключений, LTO
add_compile_options(-Os -fno-rtti -fno-exceptions -fdata-sections -ffunction-sections)
add_link_options(-Os -flto -Wl,--gc-sections -s)

find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)

set(SOURCES
    src/main.cpp
    src/platform.cpp
    src/render/glcore.cpp
    src/render/renderer.cpp
    src/render/camera.cpp
    src/world/dungeon.cpp
    src/game/combat.cpp
    src/game/enemy.cpp
    src/game/player.cpp
    src/game/tablet.cpp
)

add_executable(game ${SOURCES})
target_include_directories(game PRIVATE src ${SDL2_INCLUDE_DIRS})
target_link_libraries(game PRIVATE ${SDL2_LIBRARIES} OpenGL::GL m)
