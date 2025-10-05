/**
 * @file test_socket_integration.cpp
 * @brief Integration tests for socket functionality with real network connections
 * 
 * These tests require network access and are disabled by default.
 * Enable with: cmake -DENABLE_INTEGRATION_TESTS=ON
 * 
 * NOTE: These tests use Boost.Asio directly to test networking functionality
 * without SourceMod SDK dependencies.
 */

#include <catch2/catch_test_macros.hpp>

#ifdef ENABLE_INTEGRATION_TESTS

// Use Boost.Asio directly for testing
#ifdef USE_BOOST_ASIO
#include <boost/asio.hpp>
namespace asio = boost::asio;
using error_code = boost::system::error_code;
using tcp = boost::asio::ip::tcp;
using udp = boost::asio::ip::udp;
#else
#include <asio.hpp>
using error_code = asio::error_code;
using tcp = asio::ip::tcp;
using udp = asio::ip::udp;
#endif

#ifdef ENABLE_TLS
#include <boost/asio/ssl.hpp>
#endif

#include <memory>
#include <thread>
#include <chrono>
#include <atomic>
#include <condition_variable>
#include <string>

// Helper class to run io_service in background
class IOServiceRunner {
public:
    IOServiceRunner() : work_(std::make_unique<asio::io_service::work>(io_service_)) {
        thread_ = std::thread([this]() {
            io_service_.run();
        });
    }
    
    ~IOServiceRunner() {
        work_.reset();
        io_service_.stop();
        if (thread_.joinable()) {
            thread_.join();
        }
    }
    
    asio::io_service& get() { return io_service_; }
    
private:
    asio::io_service io_service_;
    std::unique_ptr<asio::io_service::work> work_;
    std::thread thread_;
};

// Helper to wait for async operations
class AsyncWaiter {
public:
    void wait(std::chrono::milliseconds timeout = std::chrono::milliseconds(5000)) {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait_for(lock, timeout, [this]() { return completed_; });
    }
    
    void notify() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            completed_ = true;
        }
        cv_.notify_one();
    }
    
    void reset() {
        std::lock_guard<std::mutex> lock(mutex_);
        completed_ = false;
    }
    
private:
    std::mutex mutex_;
    std::condition_variable cv_;
    bool completed_ = false;
};

TEST_CASE("TCP Connection to HTTP Server", "[integration][tcp][network]") {
    IOServiceRunner runner;
    tcp::socket socket(runner.get());
    
    AsyncWaiter waiter;
    std::atomic<bool> connect_success{false};
    std::atomic<bool> error_occurred{false};
    std::string error_message;
    
    SECTION("Connect to example.com:80") {
        tcp::resolver resolver(runner.get());
        tcp::resolver::query query("example.com", "80");
        
        resolver.async_resolve(query,
            [&](const error_code& resolve_ec, tcp::resolver::iterator endpoint_iter) {
                if (resolve_ec) {
                    error_occurred = true;
                    error_message = resolve_ec.message();
                    waiter.notify();
                    return;
                }
                
                asio::async_connect(socket, endpoint_iter,
                    [&](const error_code& connect_ec, tcp::resolver::iterator) {
                        if (connect_ec) {
                            error_occurred = true;
                            error_message = connect_ec.message();
                        } else {
                            connect_success = true;
                        }
                        waiter.notify();
                    });
            });
        
        waiter.wait();
        
        INFO("Error message: " << error_message);
        REQUIRE_FALSE(error_occurred);
        REQUIRE(connect_success);
        
        if (socket.is_open()) {
            socket.close();
        }
    }
}

TEST_CASE("TCP Connection with IPv6 Support", "[integration][tcp][ipv6][network]") {
    IOServiceRunner runner;
    
    SECTION("Resolve domain with both IPv4 and IPv6") {
        tcp::resolver resolver(runner.get());
        tcp::resolver::query query("google.com", "80", 
            tcp::resolver::query::v4_mapped | tcp::resolver::query::all_matching);
        
        AsyncWaiter waiter;
        std::atomic<bool> has_ipv4{false};
        std::atomic<bool> has_ipv6{false};
        
        resolver.async_resolve(query,
            [&](const error_code& ec, tcp::resolver::iterator it) {
                if (!ec) {
                    for (; it != tcp::resolver::iterator(); ++it) {
                        tcp::endpoint endpoint = *it;
                        if (endpoint.address().is_v4()) {
                            has_ipv4 = true;
                        } else if (endpoint.address().is_v6()) {
                            has_ipv6 = true;
                        }
                    }
                }
                waiter.notify();
            });
        
        waiter.wait();
        
        // Should have at least one IPv4 or IPv6 address
        REQUIRE((has_ipv4 || has_ipv6));
        INFO("IPv4 addresses found: " << has_ipv4.load());
        INFO("IPv6 addresses found: " << has_ipv6.load());
    }
}

TEST_CASE("TCP Send and Receive", "[integration][tcp][network]") {
    IOServiceRunner runner;
    tcp::socket socket(runner.get());
    
    AsyncWaiter connect_waiter;
    AsyncWaiter receive_waiter;
    
    std::atomic<bool> connect_success{false};
    std::atomic<bool> send_success{false};
    std::atomic<bool> receive_success{false};
    std::string received_data;
    received_data.reserve(4096);
    
    SECTION("HTTP GET request to example.com") {
        tcp::resolver resolver(runner.get());
        tcp::resolver::query query("example.com", "80");
        
        resolver.async_resolve(query,
            [&](const error_code& resolve_ec, tcp::resolver::iterator endpoint_iter) {
                if (resolve_ec) {
                    connect_waiter.notify();
                    return;
                }
                
                asio::async_connect(socket, endpoint_iter,
                    [&](const error_code& connect_ec, tcp::resolver::iterator) {
                        if (!connect_ec) {
                            connect_success = true;
                            
                            // Send HTTP GET request
                            std::string http_request = 
                                "GET / HTTP/1.1\r\n"
                                "Host: example.com\r\n"
                                "Connection: close\r\n"
                                "\r\n";
                            
                            auto buffer = std::make_shared<std::string>(http_request);
                            asio::async_write(socket, asio::buffer(*buffer),
                                [&, buffer](const error_code& write_ec, size_t bytes_sent) {
                                    if (!write_ec && bytes_sent > 0) {
                                        send_success = true;
                                        
                                        // Start receiving
                                        auto recv_buffer = std::make_shared<std::vector<char>>(4096);
                                        socket.async_read_some(asio::buffer(*recv_buffer),
                                            [&, recv_buffer](const error_code& read_ec, size_t bytes_read) {
                                                if (!read_ec && bytes_read > 0) {
                                                    received_data.append(recv_buffer->data(), bytes_read);
                                                    receive_success = true;
                                                }
                                                receive_waiter.notify();
                                            });
                                    } else {
                                        receive_waiter.notify();
                                    }
                                });
                        }
                        connect_waiter.notify();
                    });
            });
        
        connect_waiter.wait();
        REQUIRE(connect_success);
        
        if (connect_success) {
            receive_waiter.wait(std::chrono::seconds(10));
            
            REQUIRE(send_success);
            REQUIRE(receive_success);
            REQUIRE(received_data.size() > 0);
            
            // Verify HTTP response
            REQUIRE(received_data.find("HTTP/1.1") != std::string::npos);
            REQUIRE(received_data.find("200") != std::string::npos);
        }
        
        if (socket.is_open()) {
            socket.close();
        }
    }
}

TEST_CASE("UDP Send", "[integration][udp][network]") {
    IOServiceRunner runner;
    udp::socket socket(runner.get());
    
    AsyncWaiter waiter;
    std::atomic<bool> send_success{false};
    
    SECTION("UDP SendTo to public DNS server") {
        socket.open(udp::v4());
        
        // DNS query for example.com (simplified)
        unsigned char dns_query[] = {
            0x12, 0x34,  // Transaction ID
            0x01, 0x00,  // Flags: standard query
            0x00, 0x01,  // Questions: 1
            0x00, 0x00,  // Answer RRs: 0
            0x00, 0x00,  // Authority RRs: 0
            0x00, 0x00,  // Additional RRs: 0
            // Query: example.com
            0x07, 'e', 'x', 'a', 'm', 'p', 'l', 'e',
            0x03, 'c', 'o', 'm',
            0x00,        // Null terminator
            0x00, 0x01,  // Type: A
            0x00, 0x01   // Class: IN
        };
        
        udp::resolver resolver(runner.get());
        udp::resolver::query query(udp::v4(), "8.8.8.8", "53");
        
        resolver.async_resolve(query,
            [&](const error_code& resolve_ec, udp::resolver::iterator endpoint_iter) {
                if (resolve_ec) {
                    waiter.notify();
                    return;
                }
                
                auto endpoint = *endpoint_iter;
                socket.async_send_to(asio::buffer(dns_query, sizeof(dns_query)), endpoint,
                    [&](const error_code& send_ec, size_t bytes_sent) {
                        if (!send_ec && bytes_sent > 0) {
                            send_success = true;
                        }
                        waiter.notify();
                    });
            });
        
        waiter.wait();
        REQUIRE(send_success);
        
        if (socket.is_open()) {
            socket.close();
        }
    }
}

#ifdef ENABLE_TLS
TEST_CASE("TLS Connection", "[integration][tls][network]") {
    IOServiceRunner runner;
    asio::ssl::context ssl_context(asio::ssl::context::tlsv12_client);
    ssl_context.set_default_verify_paths();
    ssl_context.set_verify_mode(asio::ssl::verify_peer);
    
    asio::ssl::stream<tcp::socket> ssl_socket(runner.get(), ssl_context);
    
    AsyncWaiter waiter;
    std::atomic<bool> connect_success{false};
    std::atomic<bool> error_occurred{false};
    std::string error_message;
    
    SECTION("TLS connection to example.com:443") {
        tcp::resolver resolver(runner.get());
        tcp::resolver::query query("example.com", "443");
        
        resolver.async_resolve(query,
            [&](const error_code& resolve_ec, tcp::resolver::iterator endpoint_iter) {
                if (resolve_ec) {
                    error_occurred = true;
                    error_message = resolve_ec.message();
                    waiter.notify();
                    return;
                }
                
                asio::async_connect(ssl_socket.lowest_layer(), endpoint_iter,
                    [&](const error_code& connect_ec, tcp::resolver::iterator) {
                        if (connect_ec) {
                            error_occurred = true;
                            error_message = connect_ec.message();
                            waiter.notify();
                            return;
                        }
                        
                        // Perform TLS handshake
                        ssl_socket.async_handshake(asio::ssl::stream_base::client,
                            [&](const error_code& handshake_ec) {
                                if (handshake_ec) {
                                    error_occurred = true;
                                    error_message = handshake_ec.message();
                                } else {
                                    connect_success = true;
                                }
                                waiter.notify();
                            });
                    });
            });
        
        waiter.wait();
        
        INFO("Error message: " << error_message);
        REQUIRE_FALSE(error_occurred);
        REQUIRE(connect_success);
        
        if (ssl_socket.lowest_layer().is_open()) {
            error_code ec;
            ssl_socket.lowest_layer().close(ec);
        }
    }
}
#endif // ENABLE_TLS

TEST_CASE("Error Handling", "[integration][errors]") {
    IOServiceRunner runner;
    tcp::socket socket(runner.get());
    
    AsyncWaiter waiter;
    std::atomic<bool> error_occurred{false};
    error_code saved_error;
    
    SECTION("Connection to invalid host") {
        tcp::resolver resolver(runner.get());
        tcp::resolver::query query("this-host-does-not-exist.invalid", "80");
        
        resolver.async_resolve(query,
            [&](const error_code& ec, tcp::resolver::iterator) {
                if (ec) {
                    error_occurred = true;
                    saved_error = ec;
                }
                waiter.notify();
            });
        
        waiter.wait();
        
        REQUIRE(error_occurred);
        REQUIRE(saved_error.value() != 0);
    }
}

TEST_CASE("Concurrent Connections", "[integration][concurrent]") {
    IOServiceRunner runner;
    
    const int num_sockets = 5;
    std::vector<std::unique_ptr<tcp::socket>> sockets;
    std::vector<AsyncWaiter> waiters(num_sockets);
    std::atomic<int> success_count{0};
    
    SECTION("Multiple simultaneous connections") {
        tcp::resolver resolver(runner.get());
        tcp::resolver::query query("example.com", "80");
        
        resolver.async_resolve(query,
            [&](const error_code& resolve_ec, tcp::resolver::iterator endpoint_iter) {
                if (resolve_ec) {
                    for (auto& waiter : waiters) {
                        waiter.notify();
                    }
                    return;
                }
                
                for (int i = 0; i < num_sockets; ++i) {
                    auto socket = std::make_unique<tcp::socket>(runner.get());
                    
                    asio::async_connect(*socket, endpoint_iter,
                        [&, i](const error_code& ec, tcp::resolver::iterator) {
                            if (!ec) {
                                success_count++;
                            }
                            waiters[i].notify();
                        });
                    
                    sockets.push_back(std::move(socket));
                }
            });
        
        // Wait for all connections
        for (auto& waiter : waiters) {
            waiter.wait();
        }
        
        REQUIRE(success_count == num_sockets);
        
        // Close all sockets
        for (auto& socket : sockets) {
            if (socket->is_open()) {
                error_code ec;
                socket->close(ec);
            }
        }
    }
}

#else // ENABLE_INTEGRATION_TESTS not defined

TEST_CASE("Integration tests disabled", "[integration]") {
    WARN("Integration tests are disabled. Enable with -DENABLE_INTEGRATION_TESTS=ON");
}

#endif // ENABLE_INTEGRATION_TESTS
