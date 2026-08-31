cmake_minimum_required(VERSION 4.2)
project(Konan)

set(CMAKE_CXX_STANDARD 20)

find_package(SFML COMPONENTS Network Graphics Window Audio System CONFIG REQUIRED)

add_executable(Konan main.cpp
        Types.hpp
        simulation/ECS.hpp
        simulation/Components.hpp
        map/Grid.hpp
        map/Pathfinder.hpp
        Types.hpp
        map/MapGenerator.hpp
        engine/EventBus.hpp
        ai/BehaviorTree.hpp
        simulation/SimWorld.hpp)

target_link_libraries(Konan PRIVATE SFML::Network SFML::Graphics SFML::Window SFML::Audio SFML::System)

# Dynamic copy block using native CMake formatting (Fixes the Make e=2 error)
add_custom_command(TARGET Konan POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:SFML::Graphics>"
        "$<TARGET_FILE:SFML::Window>"
        "$<TARGET_FILE:SFML::System>"
        "$<TARGET_FILE_DIR:Konan>"
        COMMENT "Copying SFML runtime DLLs to build folder..."
)
