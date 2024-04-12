find_package(SDL2 CONFIG REQUIRED)

if(UNIX AND NOT APPLE)
    find_package(SDL_ttf REQUIRED)
    set(TTF_LIBS ${SDL_TTF_LIBRARIES})
else()
    find_package(SDL2_ttf CONFIG REQUIRED)
	set(TTF_LIBS SDL2_ttf::SDL2_ttf)
endif()

set(UTS_SOURCES
	"main.cpp"
    "Tetris/Game.cpp"
    "SDL2/Window.cpp"
    "SDL2/inputs.cpp"
    "TBP/Bot.cpp"
)


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=x86-64-v3")

add_executable(test ${UTS_SOURCES})
target_link_libraries(test PRIVATE ${SDL2_LIBRARIES} SDL2::SDL2main ${TTF_LIBS})

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/TBP)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Util)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Tetris)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/SDL2)
