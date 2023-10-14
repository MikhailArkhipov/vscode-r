cmake_minimum_required(VERSION 3.0)
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

if(NOT APPLE)
set_target_properties(Microsoft.R.Host PROPERTIES LINK_FLAGS "-static-libgcc -static-libstdc++")
else()
set_target_properties(Microsoft.R.Host PROPERTIES COMPILE_DEFINITIONS _APPLE )
set(CMAKE_OSX_DEPLOYMENT_TARGET 13.0)
endif()

include_directories("${CMAKE_SOURCE_DIR}/lib/picojson" "${CMAKE_SOURCE_DIR}/lib/picojson")

set(Boost_USE_STATIC_LIBS ON)
# Keep boost 1.65 for Ubuntu 18.
find_package(Boost 1.65.0 REQUIRED COMPONENTS date_time filesystem locale program_options regex system)
include_directories(${Boost_INCLUDE_DIRS})
target_link_libraries(Microsoft.R.Host ${Boost_LIBRARIES})

if(WIN32)
    # TODO: enable -dynamicbase 
    set_property(TARGET Microsoft.R.Host APPEND_STRING PROPERTY LINK_FLAGS " -Wl,-high-entropy-va -Wl,-nxcompat")
    
    add_definitions(-DUNICODE -D_UNICODE)
    
    target_link_libraries(Microsoft.R.Host "Bcrypt")

    find_library(MinHook_LIBRARY NAMES libMinHook.a)
    if(NOT MinHook_LIBRARY)
        message(FATAL_ERROR "MinHook not found")
    endif()
    include_directories(${MinHook_INCLUDE_DIRS})
    target_link_libraries(Microsoft.R.Host ${MinHook_LIBRARY})

    # Find R in registry, trying various keys from 4.2.x down.

    set(R_PATH "/registry")
    foreach(i RANGE 20 0 -1)
        if("${R_PATH}" STREQUAL "/registry")
            GET_FILENAME_COMPONENT(R_PATH "[HKEY_LOCAL_MACHINE\\SOFTWARE\\R-core\\R64\\4.3.${i};InstallPath]" ABSOLUTE)
        endif()
    endforeach()

    if("${R_PATH}" STREQUAL "/registry")
        message(FATAL_ERROR "R 4.3.x not found")
    endif()

    message(STATUS "Using R at ${R_PATH}")

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I\"${R_PATH}/include\"")

    # Copy MinGW dependencies to output directory alongside .exe.
    set(DLL_dependencies "libwinpthread-1.dll")
    foreach(dep ${DLL_dependencies})
        configure_file("$ENV{MSYSTEM_PREFIX}/bin/${dep}" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${dep}" COPYONLY)
    endforeach()
else()
    include_directories("/usr/share/R/include;/Library/Frameworks/R.framework/Resources/include")
    set_target_properties(Microsoft.R.Host PROPERTIES COMPILE_FLAGS "`pkg-config --cflags libzip`")
    target_link_libraries(Microsoft.R.Host pthread ${CMAKE_DL_LIBS})
endif()
