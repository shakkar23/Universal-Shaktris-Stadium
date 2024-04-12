find_package(SDL2 CONFIG REQUIRED)
find_package(SDL_ttf REQUIRED)
<<<<<<< HEAD

set(UTS_SOURCES
=======
set(UCI_SOURCES
>>>>>>> parent of e3cd325 (add text resizing, not perfect but it will do for now i guess)
	"main.cpp"
    "Tetris/Game.cpp"
    "SDL2/Window.cpp"
    "SDL2/inputs.cpp"
    "TBP/Bot.cpp"
)


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=x86-64-v3")


<<<<<<< HEAD
add_executable(test ${UTS_SOURCES})
target_link_libraries(test PRIVATE ${SDL2_LIBRARIES} SDL2::SDL2main ${SDL_TTF_LIBRARIES})
=======
add_executable("test" ${UCI_SOURCES})

target_link_libraries(test PRIVATE SDL2::SDL2 SDL2::SDL2main SDL2_ttf)
>>>>>>> parent of e3cd325 (add text resizing, not perfect but it will do for now i guess)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/TBP)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Util)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Tetris)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/SDL2)
