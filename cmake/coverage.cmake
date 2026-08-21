set(
  COVERAGE_TRACE_COMMAND
  lcov -c -q
  -o "${PROJECT_BINARY_DIR}/coverage.info"
  -d "${PROJECT_BINARY_DIR}"
  --include "${PROJECT_SOURCE_DIR}/*"
  --exclude "${PROJECT_BINARY_DIR}/*"
)

set(
  COVERAGE_HTML_COMMAND
  genhtml --legend -q
  "${PROJECT_BINARY_DIR}/coverage.info"
  -p "${PROJECT_SOURCE_DIR}"
  -o "${PROJECT_BINARY_DIR}/coverage_html"
)

add_custom_target(
  coverage
  COMMAND ${COVERAGE_TRACE_COMMAND}
  COMMAND ${COVERAGE_HTML_COMMAND}
  COMMENT "Generating coverage report"
  VERBATIM
)
