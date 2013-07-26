project(liveEEG)
#set(CMAKE_CXX_FLAGS "-Wall -O3 -std=gnu++11 -fopenmp")
set(CMAKE_CXX_FLAGS "-Wall -O3 -std=gnu++11")
cmake_minimum_required(VERSION 2.8)
#aux_source_directory(. SRC_LIST)
SET(SRC_LIST main.cpp eeg_receiver.cpp gnuplot.cpp)
add_executable(${PROJECT_NAME} ${SRC_LIST})
target_link_libraries(${PROJECT_NAME} fftw3 pthread zmq rt)

