cmake_minimum_required(VERSION 2.7)
project(R-Host)

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message(FATAL_ERROR "In-source builds are not allowed. Please invoke CMake from ./build, or use ./build.sh helper script.")
endif()

if(CMAKE_BUILD_TYPE MATCHES DEBUG)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/bin/Release)
else()
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/bin/Debug)
endif()

include_directories("${CMAKE_SOURCE_DIR}/lib/picojson" "${CMAKE_SOURCE_DIR}/lib/picojson")
#link_directories("${CMAKE_SOURCE_DIR}/lib/picojson")

#set(Boost_USE_STATIC_LIBS, ON)
find_package(Boost 1.63.0 REQUIRED COMPONENTS date_time filesystem locale program_options regex system)
include_directories(${Boost_INCLUDE_DIRS})

find_library(libzip_LIBRARY REQUIRED NAMES zip)
if(NOT libzip_LIBRARY)
    message(FATAL_ERROR "libzip not found")
endif()
include_directories(${libzip_INCLUDE_DIRS})

find_library(MinHook_LIBRARY NAMES MinHook)
if(NOT MinHook_LIBRARY)
    message(FATAL_ERROR "MinHook not found")
endif()
include_directories(${MinHook_INCLUDE_DIRS})

if(WIN32)
    add_definitions(-DUNICODE -D_UNICODE)
    
    set(WIN32_LIBRARIES "rstrmgr")

    set(R_PATH "/registry")
    foreach(i RANGE 20 1 -1)
        if("${R_PATH}" STREQUAL "/registry")
            GET_FILENAME_COMPONENT(R_PATH "[HKEY_LOCAL_MACHINE\\SOFTWARE\\R-core\\R64\\3.3.${i};InstallPath]" ABSOLUTE)
        endif()
    endforeach()

    if("${R_PATH}" STREQUAL "/registry")
        message(FATAL_ERROR "R 3.3.x not found")
    endif()

    message(STATUS "Using R at ${R_PATH}")

    set(R_INCLUDE "${R_PATH}/include")
    set(R_LIBRARIES "${R_PATH}/bin/x64/R.dll" "${R_PATH}/bin/x64/Rgraphapp.dll")
endif()

file(GLOB src "src/*.h" "src/*.cpp")

add_executable(Microsoft.R.Host ${src})

target_link_libraries(Microsoft.R.Host ${R_LIBRARIES} ${Boost_LIBRARIES} ${libzip_LIBRARY} ${MinHook_LIBRARY} ${WIN32_LIBRARIES})
