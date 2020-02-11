cmake_minimum_required (VERSION 3.1)
project (thread-pool)

find_package(Doxygen)
if (DOXYGEN_FOUND)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
  add_custom_target(doc
          ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
          WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
          COMMENT "Generating API documentation with Doxygen"
          )
endif (DOXYGEN_FOUND)

enable_testing()

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_C_FLAGS "-g -Wall -Wextra -pthread")

# http://stackoverflow.com/questions/10555706/
macro (add_executable _name)
  # invoke built-in add_executable
  _add_executable(${ARGV})
  if (TARGET ${_name})
    target_link_libraries(${_name} asyncc)
  endif()
endmacro()

include_directories(include)
add_library(asyncc STATIC src/threadpool/threadpool.c src/future/future.c)
add_executable(matrix matrix.c)
add_executable(factorial factorial.c)
add_subdirectory(test)

install(TARGETS asyncc DESTINATION .)
