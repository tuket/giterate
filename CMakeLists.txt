cmake_minimum_required(VERSION 3.8)
project(giterate)

set (CMAKE_CXX_STANDARD 17)

if(CMAKE_BUILD_TYPE MATCHES DEBUG)
	add_definitions(-DGLAD_DEBUG)
endif(CMAKE_BUILD_TYPE MATCHES DEBUG)
add_definitions(-DGLM_FORCE_RADIANS)

# this function preppends a path to all files in a list
FUNCTION(PREPEND var prefix)
SET(listVar "")
   FOREACH(f ${ARGN})
   LIST(APPEND listVar "${prefix}/${f}")
   ENDFOREACH(f)
   SET(${var} "${listVar}" PARENT_SCOPE)
ENDFUNCTION(PREPEND)

find_package(OpenGL REQUIRED)
if(OPENGL_FOUND)
	include_directories(${OPENGL_INCLUDE_DIRS})
	link_libraries(${OPENGL_LIBRARIES})
endif()

add_subdirectory(libs/glfw)
add_subdirectory(libs/glad)
add_subdirectory(libs/imgui)
add_subdirectory(libs/glm)

set(SOURCES
    "main.cpp"
    "user_api.hpp"
    "user_code.cpp")
PREPEND(SOURCES "src/" ${SOURCES})

add_executable(giterate
    ${SOURCES}
)

target_link_libraries(giterate PRIVATE glad)
target_link_libraries(giterate PRIVATE imgui)
target_link_libraries(giterate PRIVATE glfw)
target_link_libraries(giterate PRIVATE glm)