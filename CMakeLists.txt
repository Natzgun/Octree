cmake_minimum_required(VERSION 3.29)
project(Octree)

set(CMAKE_CXX_STANDARD 17)

include_directories(.)

add_executable(Octree
    OctreeColorQuantization.cc
    stb_image.h
    stb_image_write.h)
