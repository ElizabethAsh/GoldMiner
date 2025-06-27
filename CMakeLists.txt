cmake_minimum_required(VERSION 3.7)
project(BAGEL)

set(CMAKE_CXX_STANDARD 17)
add_compile_options(-Wall -Wextra)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")



set(SDL_STATIC ON)
set(SDL_SHARED OFF)
set(BUILD_SHARED_LIBS OFF)
set(SDL3IMAGE_VENDORED ON)
set(CMAKE_WIN32_EXECUTABLE FALSE)

add_executable(BAGEL
        bagel.h
        bagel_cfg.h
        main.cpp
        gold_miner_ecs.cpp
        gold_miner_ecs.h
        sprite_manager.cpp
        sprite_manager.h
        sound_manager.cpp
        sound_manager.h
        debug_draw.cpp debug_draw.h
)

# Find SFML - updated components for SFML 3.x
find_package(SFML COMPONENTS Window Graphics Audio Network REQUIRED)
if (SFML_FOUND)
        target_link_libraries(BAGEL PRIVATE SFML::Audio SFML::Graphics SFML::Window SFML::Network)
else()
        message(FATAL_ERROR "SFML not found or components are incorrect. Please ensure SFML 3.x is installed via Homebrew and components are listed correctly (e.g., Window, Graphics, Audio, Network).")
endif()

add_subdirectory(lib/SDL)
target_link_libraries(BAGEL PUBLIC SDL3-static)

add_subdirectory(lib/SDL_image)
target_link_libraries(BAGEL PUBLIC SDL3_image-static)

set(BOX2D_SAMPLES OFF)
set(BOX2D_BENCHMARKS OFF)
set(BOX2D_DOCS OFF)
set(BOX2D_PROFILE OFF)
set(BOX2D_VALIDATE OFF)
set(BOX2D_UNIT_TESTS OFF)

add_subdirectory(lib/box2d)
target_link_libraries(BAGEL PUBLIC box2d)

add_custom_command(
        TARGET BAGEL POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_SOURCE_DIR}/res"
        "$<TARGET_FILE_DIR:BAGEL>/res"
)

target_include_directories(${PROJECT_NAME} PRIVATE
        ${PROJECT_SOURCE_DIR}/lib/SDL/include
        ${PROJECT_SOURCE_DIR}/lib/SDL/include/SDL3

        ${PROJECT_SOURCE_DIR}/lib/SDL_image/include
        ${PROJECT_SOURCE_DIR}/lib/SDL_image/include/SDL3_image

        ${PROJECT_SOURCE_DIR}/lib/box2d/include
)