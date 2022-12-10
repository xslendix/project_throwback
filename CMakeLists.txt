cmake_minimum_required(VERSION 3.11) # FetchContent is available in 3.11+
project(gamejam)

# Generate compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Dependencies
find_package(raylib 4.2.0 QUIET) # QUIET or REQUIRED
if (NOT raylib_FOUND) # If there's none, fetch and build raylib
  include(FetchContent)
  FetchContent_Declare(
    raylib
    URL http://localhost:8000/4.2.0.tar.gz
  )
  #FetchContent_Declare(
  #  raylib
  #  URL https://github.com/raysan5/raylib/archive/refs/tags/4.2.0.tar.gz
  #)
  FetchContent_GetProperties(raylib)
  if (NOT raylib_POPULATED) # Have we downloaded raylib yet?
    set(FETCHCONTENT_QUIET NO)
    FetchContent_Populate(raylib)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE) # don't build the supplied examples
    add_subdirectory(${raylib_SOURCE_DIR} ${raylib_BINARY_DIR})
  endif()
endif()

# Our Project
add_executable(${PROJECT_NAME} 
  src/raylib_ex.cpp
  src/FPCamera.cpp
  src/RLFrustum.cpp
  src/main.cpp)
target_compile_options(${PROJECT_NAME} PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
)

file(COPY assets DESTINATION ${CMAKE_BINARY_DIR})

#set(raylib_VERBOSE 1)
target_link_libraries(${PROJECT_NAME} raylib)

# Web Configurations
if (${PLATFORM} STREQUAL "Web")
  # Tell Emscripten to build an example.html file.
  #set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".html")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s ALLOW_MEMORY_GROWTH=1 -s STACK_SIZE=5000000 --preload-file ../assets")
  #target_compile_options(${PROJECT_NAME} PRIVATE
  #-sALLOW_MEMORY_GROWTH
  #)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_compile_definitions(${PROJECT_NAME} PRIVATE DEBUG=1)
else()
  target_compile_definitions(${PROJECT_NAME} PRIVATE DEBUG=0)
endif()

# Checks if OSX and links appropriate frameworks (Only required on MacOS)
if (APPLE)
  target_link_libraries(${PROJECT_NAME} "-framework IOKit")
  target_link_libraries(${PROJECT_NAME} "-framework Cocoa")
  target_link_libraries(${PROJECT_NAME} "-framework OpenGL")
endif()

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)

