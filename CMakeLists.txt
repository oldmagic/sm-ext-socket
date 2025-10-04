cmake_minimum_required(VERSION 3.15)
project(socket VERSION 2.0.0 LANGUAGES CXX)

# C++17 standard required (C++20 features can be enabled with option)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(ENABLE_TLS "Enable TLS/SSL support" ON)
option(ENABLE_IPV6 "Enable IPv6 support" ON)
option(BUILD_TESTS "Build unit tests" ON)
option(USE_CPP20 "Use C++20 standard (requires compiler support)" OFF)

if(USE_CPP20)
    set(CMAKE_CXX_STANDARD 20)
endif()

# Platform detection
if(WIN32)
    set(PLATFORM_SUFFIX "windows")
elseif(UNIX AND NOT APPLE)
    set(PLATFORM_SUFFIX "linux")
else()
    message(FATAL_ERROR "Unsupported platform")
endif()

# Find required packages
find_package(Threads REQUIRED)

# Detect 32-bit build and set library path
if(CMAKE_CXX_FLAGS MATCHES "-m32" OR CMAKE_C_FLAGS MATCHES "-m32")
    set(CMAKE_LIBRARY_ARCHITECTURE "i386-linux-gnu")
    set(CMAKE_PREFIX_PATH "/usr/lib/i386-linux-gnu/cmake;${CMAKE_PREFIX_PATH}")
    set(LIB_ARCH_PATH "/usr/lib/i386-linux-gnu")
    message(STATUS "Configuring for 32-bit build")
else()
    set(CMAKE_LIBRARY_ARCHITECTURE "x86_64-linux-gnu")
    set(LIB_ARCH_PATH "/usr/lib/x86_64-linux-gnu")
    message(STATUS "Configuring for 64-bit build")
endif()

# Asio (header-only standalone or Boost.Asio)
# Try standalone Asio first, fall back to Boost.Asio
find_path(ASIO_INCLUDE_DIR asio.hpp)
if(NOT ASIO_INCLUDE_DIR)
    # For Boost.Asio, try component-based search first (works better for 64-bit)
    if(NOT CMAKE_CXX_FLAGS MATCHES "-m32" AND NOT CMAKE_C_FLAGS MATCHES "-m32")
        find_package(Boost 1.70 COMPONENTS system thread)
    else()
        # For 32-bit, just find the base package
        find_package(Boost 1.70)
    endif()
    
    if(Boost_FOUND)
        # Try to use libraries found by find_package
        if(Boost_SYSTEM_LIBRARY AND Boost_THREAD_LIBRARY)
            set(Boost_LIBRARIES ${Boost_SYSTEM_LIBRARY} ${Boost_THREAD_LIBRARY})
            message(STATUS "Found Boost libraries via find_package: ${Boost_LIBRARIES}")
        else()
            # Manual search with correct architecture path
            message(STATUS "Searching for Boost libraries manually in ${LIB_ARCH_PATH}")
            find_library(Boost_SYSTEM_LIBRARY 
                NAMES boost_system libboost_system.so.1.83.0
                PATHS ${LIB_ARCH_PATH} /usr/lib
                NO_DEFAULT_PATH)
            find_library(Boost_THREAD_LIBRARY 
                NAMES boost_thread libboost_thread.so.1.83.0
                PATHS ${LIB_ARCH_PATH} /usr/lib
                NO_DEFAULT_PATH)
            
            if(Boost_SYSTEM_LIBRARY AND Boost_THREAD_LIBRARY)
                set(Boost_LIBRARIES ${Boost_SYSTEM_LIBRARY} ${Boost_THREAD_LIBRARY})
                message(STATUS "Found Boost libraries manually: ${Boost_LIBRARIES}")
            else()
                message(FATAL_ERROR "Could not find Boost libraries for ${CMAKE_LIBRARY_ARCHITECTURE}")
            endif()
        endif()
        
        # Set include directories
        if(NOT Boost_INCLUDE_DIRS)
            set(Boost_INCLUDE_DIRS ${Boost_INCLUDE_DIR})
        endif()
    else()
        message(FATAL_ERROR "Boost not found")
    endif()
    set(USE_BOOST_ASIO ON)
    message(STATUS "Using Boost.Asio")
else()
    set(USE_BOOST_ASIO OFF)
    message(STATUS "Using standalone Asio")
endif()

# OpenSSL for TLS support
if(ENABLE_TLS)
    # For 32-bit builds, help find the correct OpenSSL
    if(CMAKE_CXX_FLAGS MATCHES "-m32" OR CMAKE_C_FLAGS MATCHES "-m32")
        set(OPENSSL_ROOT_DIR "${LIB_ARCH_PATH}")
        set(OPENSSL_CRYPTO_LIBRARY "${LIB_ARCH_PATH}/libcrypto.so.3")
        set(OPENSSL_SSL_LIBRARY "${LIB_ARCH_PATH}/libssl.so.3")
    endif()
    
    find_package(OpenSSL REQUIRED)
    message(STATUS "TLS support enabled")
    message(STATUS "OpenSSL libraries: ${OPENSSL_SSL_LIBRARY}, ${OPENSSL_CRYPTO_LIBRARY}")
endif()

# SourceMod SDK paths (can be overridden via command line)
set(SOURCEMOD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../sourcemod" CACHE PATH "Path to SourceMod SDK")
set(MMSOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../metamod-source" CACHE PATH "Path to MetaMod:Source")

# Check if SDK paths exist
if(NOT EXISTS "${SOURCEMOD_DIR}")
    message(WARNING "SourceMod SDK not found at ${SOURCEMOD_DIR}. Set SOURCEMOD_DIR to correct path.")
endif()

if(NOT EXISTS "${MMSOURCE_DIR}")
    message(WARNING "MetaMod:Source not found at ${MMSOURCE_DIR}. Set MMSOURCE_DIR to correct path.")
endif()

# Include directories
set(SOURCEMOD_INCLUDE_DIRS
    ${SOURCEMOD_DIR}/public
    ${SOURCEMOD_DIR}/public/extensions
    ${SOURCEMOD_DIR}/public/amtl/amtl
    ${SOURCEMOD_DIR}/public/amtl
    ${SOURCEMOD_DIR}/sourcepawn/include
    ${SOURCEMOD_DIR}/sourcepawn/vm
    ${SOURCEMOD_DIR}/core
    ${SOURCEMOD_DIR}/core/logic
    ${MMSOURCE_DIR}/public
    ${MMSOURCE_DIR}/core
    ${MMSOURCE_DIR}/core/sourcehook
)

# Source files
set(EXTENSION_SOURCES
    Extension.cpp
    Socket.cpp
    SocketHandler.cpp
    Callback.cpp
    CallbackHandler.cpp
    sdk/smsdk_ext.cpp
)

set(EXTENSION_HEADERS
    Extension.h
    Socket.h
    SocketHandler.h
    Callback.h
    CallbackHandler.h
    Define.h
    sdk/smsdk_ext.h
    sdk/smsdk_config.h
)

# Add TLS sources if enabled
if(ENABLE_TLS)
    list(APPEND EXTENSION_SOURCES SocketTLS.cpp)
    list(APPEND EXTENSION_HEADERS SocketTLS.h)
endif()

# Create the extension library
add_library(socket SHARED ${EXTENSION_SOURCES} ${EXTENSION_HEADERS})

# Set output name
set_target_properties(socket PROPERTIES
    OUTPUT_NAME "socket.ext"
    PREFIX ""
    SUFFIX ".so"
)

if(WIN32)
    set_target_properties(socket PROPERTIES SUFFIX ".dll")
endif()

# Include directories for target
target_include_directories(socket PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${SOURCEMOD_INCLUDE_DIRS}
)

if(USE_BOOST_ASIO)
    target_include_directories(socket PRIVATE ${Boost_INCLUDE_DIRS})
    target_link_libraries(socket PRIVATE ${Boost_LIBRARIES})
    target_compile_definitions(socket PRIVATE USE_BOOST_ASIO)
else()
    target_include_directories(socket PRIVATE ${ASIO_INCLUDE_DIR})
    target_compile_definitions(socket PRIVATE ASIO_STANDALONE)
endif()

# Link libraries
target_link_libraries(socket PRIVATE Threads::Threads)

if(ENABLE_TLS)
    target_link_libraries(socket PRIVATE OpenSSL::SSL OpenSSL::Crypto)
    target_compile_definitions(socket PRIVATE ENABLE_TLS)
endif()

if(ENABLE_IPV6)
    target_compile_definitions(socket PRIVATE ENABLE_IPV6)
endif()

# Platform-specific settings
if(WIN32)
    target_compile_definitions(socket PRIVATE
        WIN32
        _WINDOWS
        SOURCEMOD_BUILD
    )
    # Link Windows socket library
    target_link_libraries(socket PRIVATE ws2_32)
else()
    target_compile_definitions(socket PRIVATE
        _LINUX
        SOURCEMOD_BUILD
        POSIX
    )
    target_compile_options(socket PRIVATE -fPIC -fvisibility=hidden)
    
    # For 32-bit builds (common for SourceMod)
    if(CMAKE_SIZEOF_VOID_P EQUAL 4)
        target_compile_options(socket PRIVATE -m32)
        target_link_options(socket PRIVATE -m32)
    endif()
endif()

# Compiler warnings
if(MSVC)
    target_compile_options(socket PRIVATE /W4 /WX-)
else()
    target_compile_options(socket PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()

# Installation
install(TARGETS socket
    LIBRARY DESTINATION addons/sourcemod/extensions
    RUNTIME DESTINATION addons/sourcemod/extensions
)

install(FILES socket.inc
    DESTINATION addons/sourcemod/scripting/include
)

# Testing
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Print configuration summary
message(STATUS "================================")
message(STATUS "Socket Extension Configuration")
message(STATUS "================================")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "C++ Standard: C++${CMAKE_CXX_STANDARD}")
message(STATUS "Platform: ${PLATFORM_SUFFIX}")
message(STATUS "TLS Support: ${ENABLE_TLS}")
message(STATUS "IPv6 Support: ${ENABLE_IPV6}")
message(STATUS "Build Tests: ${BUILD_TESTS}")
message(STATUS "Using Boost.Asio: ${USE_BOOST_ASIO}")
message(STATUS "================================")
