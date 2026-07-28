/// Standard headers
#include <iostream>

auto gpMain(int argc, char **argv) -> int {
  (void)argc;
  (void)argv;
  std::cerr << "Unsupported. Please use the CUDA build.\n";
  return 1;
}
