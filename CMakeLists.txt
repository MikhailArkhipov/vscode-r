cmake_minimum_required(VERSION 2.7)
project(R-Host)

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message(FATAL_ERROR "In-source builds are not allowed. Please use the ./build.sh helper script.")
endif()

if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/bin/${CMAKE_BUILD_TYPE})
endif()

# TODO: add -fstack-protector
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

file(GLOB src "src/*.h" "src/*.cpp" "src/*.rc")

# Workaround for https://sourceforge.net/p/mingw-w64/bugs/622/
set_property(SOURCE "src/detours.cpp" APPEND_STRING PROPERTY COMPILE_FLAGS "-O")

add_executable(Microsoft.R.Host ${src})

set_target_properties(Microsoft.R.Host PROPERTIES LINK_FLAGS "-static-libgcc -static-libstdc++")
if("${TARGET_ARCH}" STREQUAL "x86")
    set_target_properties(Microsoft.R.Host PROPERTIES COMPILE_FLAGS "-m32" LINK_FLAGS "-m32")
endif()

include_directories("${CMAKE_SOURCE_DIR}/lib/picojson" "${CMAKE_SOURCE_DIR}/lib/picojson")

set(Boost_USE_STATIC_LIBS ON)
find_package(Boost 1.58.0 REQUIRED COMPONENTS date_time filesystem locale program_options regex system)
include_directories(${Boost_INCLUDE_DIRS})
target_link_libraries(Microsoft.R.Host ${Boost_LIBRARIES})

find_library(libzip_LIBRARY NAMES zip)
if(NOT libzip_LIBRARY)
    message(FATAL_ERROR "libzip not found")
endif()
include_directories(${libzip_INCLUDE_DIRS})
target_link_libraries(Microsoft.R.Host ${zlib_LIBRARY} ${libzip_LIBRARY})

if(WIN32)
    # TODO: enable -dynamicbase 
    set_property(TARGET Microsoft.R.Host APPEND_STRING PROPERTY LINK_FLAGS " -Wl,-high-entropy-va -Wl,-nxcompat")
    
    add_definitions(-DUNICODE -D_UNICODE)
    
    target_link_libraries(Microsoft.R.Host "rstrmgr")

    find_library(MinHook_LIBRARY NAMES libMinHook.a)
    if(NOT MinHook_LIBRARY)
        message(FATAL_ERROR "MinHook not found")
    endif()
    include_directories(${MinHook_INCLUDE_DIRS})
    target_link_libraries(Microsoft.R.Host ${MinHook_LIBRARY})

    # Find R in registry, trying various keys from 3.4.20 to 3.4.0 down.

    set(R_PATH "/registry")
    foreach(i RANGE 20 0 -1)
        if("${R_PATH}" STREQUAL "/registry")
            GET_FILENAME_COMPONENT(R_PATH "[HKEY_LOCAL_MACHINE\\SOFTWARE\\R-core\\R64\\3.4.${i};InstallPath]" ABSOLUTE)
        endif()
    endforeach()

    if("${R_PATH}" STREQUAL "/registry")
        message(FATAL_ERROR "R 3.4.x not found")
    endif()

    message(STATUS "Using R at ${R_PATH}")

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I\"${R_PATH}/include\"")

    # Copy MinGW dependencies to output directory alongside .exe.
    set(DLL_dependencies "libwinpthread-1.dll" "libzip-5.dll" "zlib1.dll")
    foreach(dep ${DLL_dependencies})
        configure_file("$ENV{MSYSTEM_PREFIX}/bin/${dep}" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${dep}" COPYONLY)
    endforeach()
else()
    find_library(R_LIBRARIES NAMES R)
    if(NOT R_LIBRARIES)
        message(FATAL_ERROR "R not found")
    endif()
    include_directories("/usr/share/R/include")
    target_link_libraries(Microsoft.R.Host ${R_LIBRARIES})

    target_link_libraries(Microsoft.R.Host pthread ${CMAKE_DL_LIBS})
endif()
