cmake_minimum_required(VERSION 3.16)
project(Football11v11 VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG        2.6.1
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(SFML)

add_executable(Football11v11
    src/main.cpp
    src/Game.cpp
    src/Field.cpp
    src/Player.cpp
    src/Ball.cpp
    src/Team.cpp
    src/AI.cpp
    src/Renderer.cpp
)

target_include_directories(Football11v11 PRIVATE include)

target_link_libraries(Football11v11 PRIVATE
    sfml-graphics
    sfml-window
    sfml-system
)

if(APPLE)
    target_link_libraries(Football11v11 PRIVATE "-framework IOKit" "-framework Cocoa")
endif()
