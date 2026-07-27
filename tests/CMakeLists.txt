project(gp-tests LANGUAGES CXX)

function(add_gp_test module name)
  add_executable(${name} ${module}/${name}.cpp)
  target_link_libraries(${name} PRIVATE libgp)
  target_compile_definitions(
    ${name}
    PRIVATE
    GP_DATA_PATH="${CMAKE_SOURCE_DIR}/data"
  )
  add_test(NAME ${name} COMMAND ${name})
endfunction()

add_gp_test(Core ArrayTest)
add_gp_test(Core DoubleBufferTest)
add_gp_test(Core GraphTest)
add_gp_test(Core MatrixTest)
add_gp_test(Core ScalarTest)
add_gp_test(Core SparseMatrixTest)
add_gp_test(Core VectorTest)
add_gp_test(Core WordsTest)

add_gp_test(QEC/Codes CodeTest)
add_gp_test(QEC/Codes ConcatenatedSurfaceCodeTest)

add_gp_test(QEC/ErrorAnalysis CircuitTest)
add_gp_test(QEC/ErrorAnalysis DriverTest)
add_gp_test(QEC/ErrorAnalysis MixerTest)
add_gp_test(QEC/ErrorAnalysis STEPGTest)

add_folders(tests)
