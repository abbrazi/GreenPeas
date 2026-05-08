if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
  message(FATAL_ERROR 
    "In-source builds are not supported. Please use a separate build directory."
  )
endif()
