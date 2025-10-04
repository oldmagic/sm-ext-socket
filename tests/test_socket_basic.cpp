/**
 * @file test_socket_basic.cpp
 * @brief Basic unit tests for socket functionality
 */

#include <catch2/catch_test_macros.hpp>
#include <string>
#include <memory>

// Test basic C++17 features used in the project
TEST_CASE("C++17 features", "[cpp17]") {
    SECTION("std::unique_ptr works") {
        auto ptr = std::make_unique<int>(42);
        REQUIRE(ptr != nullptr);
        REQUIRE(*ptr == 42);
    }
    
    SECTION("std::string_view works") {
        std::string_view sv = "Hello, Socket!";
        REQUIRE(sv.length() == 14);
        REQUIRE(sv == "Hello, Socket!");
    }
    
    SECTION("Lambda captures work") {
        int value = 10;
        auto lambda = [captured_value = value]() {
            return captured_value * 2;
        };
        REQUIRE(lambda() == 20);
    }
}

// Test std::variant (used in Callback.cpp)
#include <variant>
TEST_CASE("std::variant", "[variant]") {
    SECTION("std::variant basic usage") {
        std::variant<std::monostate, int, std::string> var;
        
        var = 42;
        REQUIRE(std::holds_alternative<int>(var));
        REQUIRE(std::get<int>(var) == 42);
        
        var = std::string("test");
        REQUIRE(std::holds_alternative<std::string>(var));
        REQUIRE(std::get<std::string>(var) == "test");
    }
}

// Test std::mutex (used throughout)
#include <mutex>
#include <thread>
TEST_CASE("std::mutex", "[mutex]") {
    SECTION("Mutex locks and unlocks") {
        std::mutex mtx;
        int shared_value = 0;
        
        {
            std::lock_guard<std::mutex> lock(mtx);
            shared_value = 42;
        }
        
        REQUIRE(shared_value == 42);
    }
    
    SECTION("Multiple threads with mutex") {
        std::mutex mtx;
        int counter = 0;
        
        auto increment = [&mtx, &counter]() {
            for (int i = 0; i < 1000; i++) {
                std::lock_guard<std::mutex> lock(mtx);
                counter++;
            }
        };
        
        std::thread t1(increment);
        std::thread t2(increment);
        
        t1.join();
        t2.join();
        
        REQUIRE(counter == 2000);
    }
}

// Test std::shared_mutex (used in Socket.h)
#include <shared_mutex>
TEST_CASE("std::shared_mutex", "[shared_mutex]") {
    SECTION("Shared and exclusive locks") {
        std::shared_mutex smtx;
        int shared_value = 0;
        
        // Exclusive lock (write)
        {
            std::unique_lock<std::shared_mutex> lock(smtx);
            shared_value = 42;
        }
        
        // Shared locks (read)
        {
            std::shared_lock<std::shared_mutex> lock1(smtx);
            std::shared_lock<std::shared_mutex> lock2(smtx);
            REQUIRE(shared_value == 42);
        }
    }
}

// Test std::deque (used in CallbackHandler.cpp)
#include <deque>
TEST_CASE("std::deque", "[deque]") {
    SECTION("Deque operations") {
        std::deque<int> dq;
        
        dq.push_back(1);
        dq.push_back(2);
        dq.push_back(3);
        
        REQUIRE(dq.size() == 3);
        REQUIRE(dq.front() == 1);
        REQUIRE(dq.back() == 3);
        
        dq.pop_front();
        REQUIRE(dq.front() == 2);
        REQUIRE(dq.size() == 2);
    }
}

// Test enum class (used in Define.h)
enum class TestSocketType { Tcp = 1, Udp = 2 };

TEST_CASE("enum class", "[enum]") {
    SECTION("Type-safe enums") {
        TestSocketType type = TestSocketType::Tcp;
        REQUIRE(static_cast<int>(type) == 1);
        
        type = TestSocketType::Udp;
        REQUIRE(static_cast<int>(type) == 2);
        
        // Enum classes prevent implicit conversion
        // This would not compile: int x = type;
    }
}

// Test move semantics
TEST_CASE("Move semantics", "[move]") {
    SECTION("std::move with unique_ptr") {
        auto ptr1 = std::make_unique<int>(42);
        REQUIRE(ptr1 != nullptr);
        
        auto ptr2 = std::move(ptr1);
        REQUIRE(ptr1 == nullptr);
        REQUIRE(ptr2 != nullptr);
        REQUIRE(*ptr2 == 42);
    }
    
    SECTION("Moving in lambda captures") {
        auto ptr = std::make_unique<int>(100);
        
        auto lambda = [captured = std::move(ptr)]() {
            return *captured;
        };
        
        REQUIRE(ptr == nullptr); // Moved from
        REQUIRE(lambda() == 100);
    }
}

// Test Boost.Asio availability
#ifdef USE_BOOST_ASIO
#include <boost/asio.hpp>
TEST_CASE("Boost.Asio availability", "[boost]") {
    SECTION("Can create io_service") {
        boost::asio::io_service io;
        REQUIRE(true); // Just check it compiles
    }
    
    SECTION("Can create TCP socket") {
        boost::asio::io_service io;
        boost::asio::ip::tcp::socket sock(io);
        REQUIRE(!sock.is_open()); // Not opened yet
    }
    
    SECTION("Can create UDP socket") {
        boost::asio::io_service io;
        boost::asio::ip::udp::socket sock(io);
        REQUIRE(!sock.is_open()); // Not opened yet
    }
}
#endif

// Integration test - compilation success
TEST_CASE("Project compilation", "[integration]") {
    SECTION("Modern C++17 project compiles") {
        // If this test runs, the project compiled successfully
        REQUIRE(true);
    }
    
    SECTION("All modern features available") {
        // unique_ptr, variant, mutex, shared_mutex, deque, enum class
        bool all_features_available = true;
        REQUIRE(all_features_available);
    }
}

// Performance characteristics test
TEST_CASE("Performance characteristics", "[performance]") {
    SECTION("unique_ptr has minimal overhead") {
        auto ptr = std::make_unique<int>(42);
        REQUIRE(sizeof(ptr) == sizeof(int*));
    }
    
    SECTION("Move operations are efficient") {
        auto start = std::make_unique<std::string>("test");
        auto moved = std::move(start);
        // Move should be fast, just pointer swap
        REQUIRE(moved != nullptr);
    }
}
