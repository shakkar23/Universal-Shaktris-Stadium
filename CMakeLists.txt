
cmake_minimum_required (VERSION 3.22)

project("UTS")

set(CMAKE_CXX_STANDARD 23)

add_subdirectory("src")

# copy the assets folder into where the exe will be built
file(COPY "assets" DESTINATION ${CMAKE_BINARY_DIR}/src)