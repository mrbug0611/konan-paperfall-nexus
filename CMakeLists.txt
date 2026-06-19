cmake_minimum_required(VERSION 4.2)
project(Konan)

set(CMAKE_CXX_STANDARD 20)


find_package(SFML COMPONENTS Network Graphics Window Audio System CONFIG REQUIRED)

add_executable(Konan main.cpp)

target_link_libraries(Konan PRIVATE SFML::Network SFML::Graphics SFML::Window SFML::Audio SFML::System)



