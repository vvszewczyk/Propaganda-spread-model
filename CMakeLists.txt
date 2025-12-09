cmake_minimum_required(VERSION 3.20)
project(PropagandaSpreadModel LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Charts Svg)

add_executable(PropagandaSpreadModel
  src/main.cpp
  src/MainWindow.cpp
  src/UsMap.cpp
  src/Simulation.cpp
  src/GridWidget.cpp
  src/Constants.cpp
  include/MainWindow.hpp
  #include/UsMap.hpp
  include/GridWidget.hpp
)
target_include_directories(PropagandaSpreadModel PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(PropagandaSpreadModel PRIVATE Qt6::Widgets Qt6::Charts Qt6::Svg)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_custom_target(copy-compile-commands ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_BINARY_DIR}/compile_commands.json
            ${CMAKE_SOURCE_DIR}/compile_commands.json
)

if (MSVC)
  target_compile_options(PropagandaSpreadModel PRIVATE /W4 /permissive-)
else()
  target_compile_options(PropagandaSpreadModel PRIVATE -Wall -Wextra -Wpedantic)
endif()

if (WIN32)
    add_custom_command(TARGET PropagandaSpreadModel POST_BUILD
        COMMAND ${Qt6_DIR}/../../../bin/windeployqt.exe $<TARGET_FILE:PropagandaSpreadModel>
        COMMENT "Running windeployqt..."
    )
endif()
