cmake_minimum_required (VERSION 3.13)

project (Tutorial)

# C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 정적 라이브러리
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")


# Set flags
# Build with multiple processes
add_compile_options(/MP)

#add_compile_definitions(UNICODE _UNICODE USE_CONNECT)
add_compile_definitions(UNICODE _UNICODE)


# Include libraries
# libevent
set(EVENT__DISABLE_OPENSSL   YES CACHE BOOL   "Disable OpenSSL in LibEvent")
set(EVENT__DISABLE_BENCHMARK YES CACHE BOOL   "Disable LibEvent benchmarks")
set(EVENT__DISABLE_TESTS     YES CACHE BOOL   "Disable LibEvent tests")
set(EVENT__DISABLE_REGRESS   YES CACHE BOOL   "Disable LibEvent regression tests")
set(EVENT__DISABLE_SAMPLES   YES CACHE BOOL   "Disable LibEvent samples")
set(EVENT__LIBRARY_TYPE "STATIC" CACHE STRING "Use static LibEvent libraries")

# jsoncpp
set(JSONCPP_WITH_TESTS OFF CACHE BOOL "Compile and (for jsoncpp_check) run JsonCpp test executables")
set(JSONCPP_WITH_POST_BUILD_UNITTEST OFF CACHE BOOL "Automatically run unit-tests as a post build step")
set(JSONCPP_WITH_PKGCONFIG_SUPPORT OFF CACHE BOOL "Generate and install .pc files")

# boost
#SET(CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH} "C:/Program Files/boost_1_75_0")
#SET(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} "C:/Program Files/boost_1_75_0/stage/lib")
#SET(CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH} "E:/Program/boost_1_75_0/boost_1_75_0")
#SET(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} "E:/Program/boost_1_75_0/boost_1_75_0/stage/lib")
find_package(Boost)
if(Boost_FOUND)
	message ("Boost_FOUND")
	include_directories(${Boost_INCLUDE_DIRS})
endif()

# mysql
include_directories("C:/Program Files/MySQL/Connector C++ 8.0/include")
include_directories("C:/Program Files/MySQL/MySQL Server 8.0/include")
link_directories("C:/Program Files/MySQL/Connector C++ 8.0/lib64/vs14")
link_directories("C:/Program Files/MySQL/MySQL Server 8.0/lib")


# libs
set(DEPENDENCIES fmt jsoncpp libevent soci openssl zlib)
foreach(DEPENDENCY ${DEPENDENCIES})
	if (NOT EXISTS "${PROJECT_SOURCE_DIR}/lib/${DEPENDENCY}/CMakeLists.txt")
		message(FATAL_ERROR "lib/${DEPENDENCY} is not found.")
	endif()

    add_subdirectory("lib/${DEPENDENCY}" EXCLUDE_FROM_ALL)
endforeach()


add_executable(${CMAKE_PROJECT_NAME})
add_subdirectory(src)

set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT ${CMAKE_PROJECT_NAME})

target_compile_options(
	${CMAKE_PROJECT_NAME} PRIVATE

	#-fsigned-char
	#-ffast-math
	#-Wall -Wextra
	#-Wno-unused-parameter
)

# Link libraries
target_include_directories(
	${CMAKE_PROJECT_NAME} SYSTEM PRIVATE

	lib/TCLAP/include
	lib
)

target_link_libraries(
	${CMAKE_PROJECT_NAME} PRIVATE

	event_core
	event_extra
	fmt::fmt
	jsoncpp_lib
	soci_core
	soci_mysql
	libssl 
	libcrypto 
	zlib
)

set_property(TARGET jsoncpp_lib PROPERTY PROJECT_LABEL "jsoncpp")

# Grouping folders
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set_target_properties(
	event_core_static
	event_extra_static
	fmt
	jsoncpp_lib
	soci_core
	soci_mysql
	openssl
	zlib

	PROPERTIES FOLDER Libraries
)

get_property(TARGET_SOURCE_FILES TARGET ${CMAKE_PROJECT_NAME} PROPERTY SOURCES)
source_group(TREE "${PROJECT_SOURCE_DIR}/src" FILES ${TARGET_SOURCE_FILES})





#set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")

#message ("cxx Flags: " ${CMAKE_CXX_FLAGS})

