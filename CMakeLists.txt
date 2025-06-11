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

add_executable(BAGEL
        bagel.h
        bagel_cfg.h
        gold_miner_ecs.cpp
        gold_miner_ecs.h
        sprite_manager.cpp
        sprite_manager.h
)

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
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
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

