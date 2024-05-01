find_package(SDL2 CONFIG REQUIRED)

if(UNIX AND NOT APPLE)
    find_package(SDL_ttf REQUIRED)
    set(TTF_LIBS SDL2_ttf)
else()
    find_package(SDL2_ttf CONFIG REQUIRED)
	set(TTF_LIBS SDL2_ttf::SDL2_ttf)
endif()

set(UTS_SOURCES
    "main.cpp"
    "Shaktris/Game.cpp"
    "SDL2/Window.cpp"
    "SDL2/inputs.cpp"
    "TBP/Bot.cpp"
)


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=x86-64-v3")

add_executable(sdl2_stadium ${UTS_SOURCES})
target_link_libraries(sdl2_stadium PRIVATE ${SDL2_LIBRARIES} SDL2::SDL2main ${TTF_LIBS})

set(UTS_CLI_SOURCES
    "Shaktris/Game.cpp"
    "cli_main.cpp"
    "TBP/Bot.cpp"
)

add_executable(sdl2_stadium_cli ${UTS_CLI_SOURCES} )

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/TBP)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Util)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Shaktris)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/SDL2)
