cmake_minimum_required(VERSION 3.20)
project(ProjectWire VERSION 0.3.0 LANGUAGES CXX C)

# Set C++20 Standard with strict memory safety & compiler warning flags
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler Warning Flags for Security & Memory Safety
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow
        -Wnon-virtual-dtor
        -Wold-style-cast
        -Wcast-align
        -Wunused
        -Woverloaded-virtual
        -Wconversion
        -Wsign-conversion
        -Wnull-dereference
        -Wdouble-promotion
        -Wformat=2
        -fstack-protector-strong
    )
endif()

# Include Directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Core Libraries
add_library(wire_crypto STATIC
    src/crypto/chacha20_prng.cpp
    src/crypto/knock_engine.cpp
    src/crypto/double_ratchet.cpp
)

add_library(wire_network STATIC
    src/network/tls_mimicry.cpp
    src/network/discovery_engine.cpp
)

add_library(wire_ledger STATIC
    src/ledger/merkle_dag.cpp
)

# Protection Domains Executables (seL4 Microkit binaries)
add_executable(wire_vault_pd src/pd/vault_pd.cpp)
target_link_libraries(wire_vault_pd PRIVATE wire_crypto wire_network wire_ledger)

add_executable(wire_network_pd src/pd/network_pd.cpp)
target_link_libraries(wire_network_pd PRIVATE wire_crypto wire_network)

add_executable(wire_monitor_pd src/pd/monitor_pd.cpp)
target_link_libraries(wire_monitor_pd PRIVATE wire_crypto wire_network)

# Host Unit Test Executables
enable_testing()

add_executable(test_chacha20_prng tests/test_chacha20_prng.cpp)
target_link_libraries(test_chacha20_prng PRIVATE wire_crypto)
add_test(NAME ChaCha20PRNGTest COMMAND test_chacha20_prng)

add_executable(test_prng_security_and_perf tests/test_prng_security_and_perf.cpp)
target_link_libraries(test_prng_security_and_perf PRIVATE wire_crypto)
add_test(NAME PRNGSecurityAndPerfTest COMMAND test_prng_security_and_perf)

add_executable(test_knock_engine tests/test_knock_engine.cpp)
target_link_libraries(test_knock_engine PRIVATE wire_crypto)
add_test(NAME KnockEngineTest COMMAND test_knock_engine)

add_executable(test_tls_mimicry tests/test_tls_mimicry.cpp)
target_link_libraries(test_tls_mimicry PRIVATE wire_network)
add_test(NAME TLSMimicryTest COMMAND test_tls_mimicry)

add_executable(test_discovery_engine tests/test_discovery_engine.cpp)
target_link_libraries(test_discovery_engine PRIVATE wire_crypto wire_network)
add_test(NAME DiscoveryEngineTest COMMAND test_discovery_engine)

add_executable(test_double_ratchet tests/test_double_ratchet.cpp)
target_link_libraries(test_double_ratchet PRIVATE wire_crypto)
add_test(NAME DoubleRatchetTest COMMAND test_double_ratchet)

add_executable(test_merkle_dag tests/test_merkle_dag.cpp)
target_link_libraries(test_merkle_dag PRIVATE wire_ledger)
add_test(NAME MerkleDAGTest COMMAND test_merkle_dag)

add_executable(test_capability_severing tests/test_capability_severing.cpp)
target_link_libraries(test_capability_severing PRIVATE wire_crypto)
add_test(NAME CapabilitySeveringTest COMMAND test_capability_severing)
