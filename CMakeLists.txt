cmake_minimum_required(VERSION 3.27)

include(cmake/prelude.cmake)

project(
  GreenPeas
  VERSION 0.1.0
  DESCRIPTION "GreenPeas is a C++/CUDA toolchain for the high-speed, just-in-time compilation of decoding hypergraphs."
  LANGUAGES CXX
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)
include(cmake/dependencies.cmake)

add_library(libgp INTERFACE)

option(GP_USE_CUDA "Build GreenPeas with CUDA" OFF)

set(
  GP_CUDA_ARCHITECTURES
  "86"
  CACHE STRING
  "CUDA architectures (default: 86/Ampere)."
)

if(GP_USE_CUDA)
  set(CMAKE_CUDA_ARCHITECTURES "${GP_CUDA_ARCHITECTURES}")

  find_package(CUDAToolkit REQUIRED)
  enable_language(CUDA)

  set_target_properties(libgp PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CUDA_STANDARD 20
    CUDA_STANDARD_REQUIRED ON
    CUDA_SEPARABLE_COMPILATION ON
    CUDA_EXTENSIONS OFF
  )

  target_link_libraries(libgp INTERFACE CUDA::cudart)
  target_compile_definitions(libgp INTERFACE GP_HAS_CUDA=1)
  set(GP_EXT cu)
else()
  message(STATUS "CUDA support is disabled. Building CPU-only version.")
  set(GP_EXT cpp)
endif()

target_link_libraries(libgp INTERFACE libstim)

target_include_directories(
  libgp
  ${warning_guard}
  INTERFACE
  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
)

target_compile_features(libgp INTERFACE cxx_std_20)

option(GP_BUILD_PYTHON "Build Python bindings with pybind11" OFF)

if(GP_BUILD_PYTHON)
  include(cmake/python.cmake)
endif()

set(GP_SOURCES
  source/main.cpp
  "source/GreenPeasMain.${GP_EXT}"
)

add_executable(gp ${GP_SOURCES})

if(GP_USE_CUDA)
  set_target_properties(gp PROPERTIES
    CUDA_STANDARD 20
    CUDA_ARCHITECTURES "${GP_CUDA_ARCHITECTURES}"
  )
endif()

target_link_libraries(gp PRIVATE libgp CLI11::CLI11)

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()

if(NOT GP_DEVELOPER_MODE)
  return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
  message(
    AUTHOR_WARNING
    "Developer mode is intended for developers of GreenPeas."
  )
endif()

include(cmake/developer-mode.cmake)

target_compile_options(
  libgp
  INTERFACE
  $<$<BOOL:${GP_ENABLE_COVERAGE}>:--coverage>
)
target_link_options(
  libgp
  INTERFACE
  $<$<BOOL:${GP_ENABLE_COVERAGE}>:--coverage>
)
