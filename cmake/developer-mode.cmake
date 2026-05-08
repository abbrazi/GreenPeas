include(cmake/folders.cmake)

include(CTest)
if(BUILD_TESTING)
  add_subdirectory(tests)
endif()

option(GP_BUILD_DOCS "Build documentation using Doxygen and m.css" OFF)
if(GP_BUILD_DOCS)
  include(cmake/docs.cmake)
endif()

option(GP_ENABLE_COVERAGE "Enable coverage support separate from CTest's" OFF)
if(GP_ENABLE_COVERAGE)
  include(cmake/coverage.cmake)
endif()

include(cmake/lint-targets.cmake)
include(cmake/spell-targets.cmake)

add_folders(Project)
