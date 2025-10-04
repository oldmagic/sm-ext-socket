cmake_minimum_required(VERSION 3.15)

# Find or download Catch2
find_package(Catch2 3 QUIET)

if(NOT Catch2_FOUND)
    message(STATUS "Catch2 not found, fetching from GitHub...")
    include(FetchContent)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.4.0
    )
    FetchContent_MakeAvailable(Catch2)
endif()

# Test executable
add_executable(socket_tests
    test_main.cpp
    test_socket_basic.cpp
)

target_link_libraries(socket_tests PRIVATE
    Catch2::Catch2WithMain
    Threads::Threads
)

target_compile_definitions(socket_tests PRIVATE USE_BOOST_ASIO)

if(USE_BOOST_ASIO)
    target_link_libraries(socket_tests PRIVATE Boost::system Boost::thread)
    target_include_directories(socket_tests PRIVATE ${Boost_INCLUDE_DIRS})
else()
    target_include_directories(socket_tests PRIVATE ${ASIO_INCLUDE_DIR})
    target_compile_definitions(socket_tests PRIVATE ASIO_STANDALONE)
endif()

if(ENABLE_TLS)
    target_link_libraries(socket_tests PRIVATE OpenSSL::SSL OpenSSL::Crypto)
    target_compile_definitions(socket_tests PRIVATE ENABLE_TLS)
endif()

target_include_directories(socket_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# Register tests with CTest
include(CTest)
include(Catch)
catch_discover_tests(socket_tests)

# Add custom test target
add_custom_target(run_tests
    COMMAND socket_tests
    DEPENDS socket_tests
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running unit tests..."
)
