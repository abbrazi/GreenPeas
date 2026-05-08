find_package(Python REQUIRED COMPONENTS Interpreter Development.Module)

find_package(pybind11 CONFIG QUIET)
if(NOT pybind11_FOUND)
  include(FetchContent)
  FetchContent_Declare(
    pybind11
    GIT_REPOSITORY https://github.com/pybind/pybind11.git
    GIT_TAG v2.13.6
  )
  FetchContent_MakeAvailable(pybind11)
endif()

set(GPPY_SOURCES
  "${PROJECT_SOURCE_DIR}/source/GreenPeasPy.${GP_EXT}"
  "${PROJECT_SOURCE_DIR}/source/QEC/Codes/Codes.pybind.cpp"
  "${PROJECT_SOURCE_DIR}/source/QEC/ErrorAnalysis/ErrorAnalysis.pybind.${GP_EXT}"
)

pybind11_add_module(_gppy ${GPPY_SOURCES})

target_link_libraries(_gppy PRIVATE libgp)

if(GP_USE_CUDA)
  set_target_properties(_gppy PROPERTIES
    CUDA_STANDARD 20
    CUDA_ARCHITECTURES "${GP_CUDA_ARCHITECTURES}"
  )
endif()

set_target_properties(
  _gppy
  PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/python/greenpeas"
)

install(TARGETS _gppy LIBRARY DESTINATION greenpeas COMPONENT python)
install(DIRECTORY "${PROJECT_SOURCE_DIR}/data/"
        DESTINATION greenpeas/data
        COMPONENT python)
