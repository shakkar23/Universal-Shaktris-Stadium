find_package(SDL2 CONFIG REQUIRED)
find_package(SDL2_ttf CONFIG REQUIRED)

set(UCI_SOURCES
	"main.cpp"
    "Tetris/Game.cpp"
    "SDL2/Window.cpp"
    "SDL2/inputs.cpp"
    "TBP/Bot.cpp"
)


 set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=x86-64-v3")


add_executable("test" ${UCI_SOURCES})

target_link_libraries(test PRIVATE SDL2::SDL2 SDL2::SDL2main SDL2_ttf::SDL2_ttf)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/TBP)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Util)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Tetris)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/SDL2)
