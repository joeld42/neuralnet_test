cmake_minimum_required(VERSION 3.1...3.12)
if(${CMAKE_VERSION} VERSION_LESS 3.12)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

project(tictoe VERSION 0.0.1
                  DESCRIPTION "TictacToe Test"
                  LANGUAGES CXX)

set ( RAYLIB_HOME /Users/joeld/Toolkits/raylib/raylib )
set ( RAYGUI_HOME /Users/joeld/Toolkits/raylib/raygui )

include_directories(
	${RAYLIB_HOME}/src
	${RAYGUI_HOME}/src
	/usr/local/include
)

link_directories(
	${RAYLIB_HOME}/build/src/
	/usr/local/Cellar/glfw3/3.2.1/lib 	
)

set ( LIBS ${LIBS}
		"-framework IOKit"
		"-framework OpenGL"
		"-framework OpenAl"
		"-framework Cocoa"
		"-framework AudioToolbox"
		raylib
		#glfw3
      )

set ( CMAKE_CXX_FLAGS -Wno-nullability-completeness )
set ( CMAKE_CXX_STANDARD 14)

# =========================================================
# reorganized stuff
# =========================================================
add_executable (tk_game_tictoe 
				tk_game_tictoe.cpp
				tk_evalgame.cpp
				genann.cpp
				)

target_compile_options( tk_game_tictoe PUBLIC -fno-rtti)
target_link_libraries( tk_game_tictoe ${LIBS} )
set_target_properties( tk_game_tictoe PROPERTIES 
	 					RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/ )

# =========================================================
# Original test game
# =========================================================
add_executable (tictoe 
				tictoe.cpp
				genann.cpp
				)

target_compile_options( tictoe PUBLIC -fno-rtti)
target_link_libraries( tictoe ${LIBS} )
set_target_properties( tictoe PROPERTIES 
	 					RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/ )
