cmake_minimum_required (VERSION 3.22)
project (DPE)

set(CMAKE_CXX_STANDARD 20)

include(CheckLanguage)
check_language(CUDA)
if (CMAKE_CUDA_COMPILER)
    enable_language(CUDA)
    message(" -- CUDA FOUND")
    set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -O3 --use_fast_math --maxrregcount=128 --ptxas-options=-v --compiler-options -Wall")
    # set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -lineinfo") # for CUDA profiling!
else()
    message(FATAL_ERROR "-- CUDA NOT FOUND: Fatal error")
endif()

# recommand >=3.3.0
# find_package(OpenCV REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(OpenCV REQUIRED core imgproc imgcodecs)
# recommand >=1.62.0
# recommand >=10.2.

include_directories(.)

if(CMAKE_COMPILER_IS_GNUCXX)
    add_definitions(-pthread)
    add_definitions(-Wall)
    add_definitions(-Wextra)
    add_definitions(-pedantic)
    add_definitions(-Wno-unused-function)
    add_definitions(-Wno-switch)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -ffast-math -march=native")
endif()

cuda_add_executable(
    DPE
    main.h
    DPE.h
    DPE.cpp
    DPE.cu
    main.cpp
)

target_link_libraries(DPE
    ${Boost_LIBRARIES}
    ${OpenCV_LIBS}
)

target_include_directories(DPE
    PUBLIC
    ${OpenCV_INCLUDE_DIRS}
)

set_target_properties(DPE
    PROPERTIES CUDA_SEPARABLE_COMPILATION ON)
target_include_directories(
    DPE PUBLIC
    ${CMAKE_CUDA_TOOLKIT_INCLUDE_DIRECTORIES}
)