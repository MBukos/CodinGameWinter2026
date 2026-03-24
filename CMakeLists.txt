cmake_minimum_required(VERSION 4.1)
project(CodinGame2026)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
# Jeśli używasz Windowsa i MinGW
if(WIN32)
    # Flaga -static jest "agresywna" i zazwyczaj załatwia wszystko za jednym zamachem
    set(CMAKE_EXE_LINKER_FLAGS "-static")
endif()
#add_executable(CodinGame2026 main.cpp)
#add_executable(hybrid hybrid.cpp)
add_executable(MyBot bot.cpp)
#add_executable(SebaBot seba_bot.cpp)
