find_package(SDL2 CONFIG REQUIRED)

if(UNIX AND NOT APPLE)
    find_package(SDL_ttf REQUIRED)
    set(TTF_LIBS SDL2_ttf)
else()
    find_package(SDL2_ttf CONFIG REQUIRED)
	set(TTF_LIBS SDL2_ttf::SDL2_ttf)
endif()

include(FetchContent)
FetchContent_Declare(
  ixwebsocket
  GIT_REPOSITORY https://github.com/machinezone/IXWebSocket.git
  GIT_TAG v11.4.6
  GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(ixwebsocket)

#find_package(ixwebsocket CONFIG REQUIRED)


set(COMMON_SOURCES
    "Shaktris/Game.cpp"
    "SDL2/Window.cpp"
    "SDL2/inputs.cpp"
    "TBP/Bot.cpp"
)



set(UTS_SOURCES
    "stadium_gui.cpp"
    ${COMMON_SOURCES}
)


if( NOT WIN32)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=x86-64-v3")
endif()

add_executable(sdl2_stadium ${UTS_SOURCES})
target_link_libraries(sdl2_stadium PRIVATE SDL2::SDL2 SDL2::SDL2main ${TTF_LIBS})


set(UTS_CLI_SOURCES
    "Shaktris/Game.cpp"
    "stadium_cli.cpp"
    "TBP/Bot.cpp"
)

add_executable(sdl2_stadium_cli ${UTS_CLI_SOURCES} )



set(VISUALIZER_SOURCES
    "tbp_visualizer.cpp"
    ${COMMON_SOURCES}
)
add_executable(tbp_visualizer ${VISUALIZER_SOURCES})
target_link_libraries(tbp_visualizer PRIVATE SDL2::SDL2 SDL2::SDL2main ${TTF_LIBS})

set(DATASET_VIS_SOURCES
    "dataset_visualizer.cpp"
    ${COMMON_SOURCES}
)

add_executable(dataset_visualizer ${DATASET_VIS_SOURCES})
target_link_libraries(dataset_visualizer PRIVATE SDL2::SDL2 SDL2::SDL2main ${TTF_LIBS})

set(BOTRIS_SOURCES
    "bottris_stadium.cpp"
    "SDL2/BotrisVisualizer.cpp"
    "TBP/Botris.cpp"
    ${COMMON_SOURCES}
)

add_executable(sdl2_bottris_stadium ${BOTRIS_SOURCES})
target_link_libraries(sdl2_bottris_stadium PRIVATE SDL2::SDL2 SDL2::SDL2main ${TTF_LIBS} ixwebsocket::ixwebsocket)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/TBP)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Util)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Shaktris)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/SDL2)
