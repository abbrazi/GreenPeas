include(FetchContent)

FetchContent_Declare(
  CLI11
  GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
  GIT_TAG v2.6.2
)
FetchContent_MakeAvailable(CLI11)

FetchContent_Declare(
  stim
  GIT_REPOSITORY https://github.com/quantumlib/Stim.git
  GIT_TAG v1.15.0
)
FetchContent_MakeAvailable(stim)

set(BUILD_SHARED_LIBS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

FetchContent_Declare(
  tesseract
  GIT_REPOSITORY https://github.com/quantumlib/tesseract-decoder.git
  GIT_TAG 7a522a2269eb2f3304079040c8699cfc50771493
)
FetchContent_MakeAvailable(tesseract)
