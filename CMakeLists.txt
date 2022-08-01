cmake_minimum_required(VERSION 3.5)

set(CMAKE_C_COMPILER C:/msys64/usr/bin/gcc.exe)
set(CMAKE_CXX_COMPILER C:/msys64/usr/bin/g++.exe)
project(TUNDRA_ENGINE LANGUAGES C CXX)

# -Wall for warnings
add_compile_options(-std=c++17 -O0 -g)

file(GLOB TUNDRA_SRC CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/*.cpp)

add_library(TundraEngine STATIC ${TUNDRA_SRC})
target_include_directories(TundraEngine PRIVATE ${PROJECT_SOURCE_DIR}/../include ${PROJECT_SOURCE_DIR}/../include/Bullet ${PROJECT_SOURCE_DIR}/../include/Bullet/BulletCollision/CollisionShapes ${PROJECT_SOURCE_DIR}/../include/Bullet/BulletCollision/CollisionDispatch)