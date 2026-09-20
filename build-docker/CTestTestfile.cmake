# CMake generated Testfile for 
# Source directory: /workspace
# Build directory: /workspace/build-docker
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test([=[ChaCha20PRNGTest]=] "/workspace/build-docker/test_chacha20_prng")
set_tests_properties([=[ChaCha20PRNGTest]=] PROPERTIES  _BACKTRACE_TRIPLES "/workspace/CMakeLists.txt;52;add_test;/workspace/CMakeLists.txt;0;")
add_test([=[PRNGSecurityAndPerfTest]=] "/workspace/build-docker/test_prng_security_and_perf")
set_tests_properties([=[PRNGSecurityAndPerfTest]=] PROPERTIES  _BACKTRACE_TRIPLES "/workspace/CMakeLists.txt;56;add_test;/workspace/CMakeLists.txt;0;")
