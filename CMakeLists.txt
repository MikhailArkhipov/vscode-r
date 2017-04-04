cmake_minimum_required(VERSION 2.7)
project(R-Host)

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message(FATAL_ERROR "In-source builds are not allowed. Please use the ./build.sh helper script.")
endif()

if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/bin/${CMAKE_BUILD_TYPE})
endif()

file(GLOB src "src/*.h" "src/*.cpp" "src/*.rc")

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

if(WIN32)
    find_library(libzip_LIBRARY NAMES zip)
else()
    find_library(libzip_LIBRARY NAMES libzip.a)
endif()    

if(NOT libzip_LIBRARY)
    message(FATAL_ERROR "libzip not found")
endif()
include_directories(${libzip_INCLUDE_DIRS})
target_link_libraries(Microsoft.R.Host ${zlib_LIBRARY} ${libzip_LIBRARY})

if(WIN32)
    add_definitions(-DUNICODE -D_UNICODE)
    
    target_link_libraries(Microsoft.R.Host "rstrmgr")

    find_library(MinHook_LIBRARY NAMES libMinHook.a)
    if(NOT MinHook_LIBRARY)
        message(FATAL_ERROR "MinHook not found")
    endif()
    include_directories(${MinHook_INCLUDE_DIRS})
    target_link_libraries(Microsoft.R.Host ${MinHook_LIBRARY})

    # Find R in registry, trying various keys from 3.3.20 to 3.3.1 down.

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

    set(CMAKE_CXX_FLAGS "-I\"${R_PATH}/include\"")
    target_link_libraries(Microsoft.R.Host "${R_PATH}/bin/x64/R.dll" "${R_PATH}/bin/x64/Rgraphapp.dll")

    # Copy MinGW dependencies to output directory alongside .exe.
    set(DLL_dependencies "libwinpthread-1.dll" "libzip-5.dll")
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

    target_link_libraries(Microsoft.R.Host pthread)
endif()
