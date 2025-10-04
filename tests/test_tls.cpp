#ifdef ENABLE_TLS

#include <iostream>
#include <memory>
#include <cassert>
#include "SocketTLS.h"
#include "SocketHandler.h"
#include "CallbackHandler.h"

// Test TLS socket creation
void test_tls_socket_creation() {
    std::cout << "Testing TLS socket creation..." << std::endl;
    
    auto sslContext = socketHandler.GetSSLContext();
    assert(sslContext != nullptr);
    
    auto* tlsSocket = socketHandler.CreateTLSSocket();
    assert(tlsSocket != nullptr);
    assert(!tlsSocket->IsOpen());
    
    auto* wrapper = socketHandler.GetSocketWrapper(tlsSocket);
    assert(wrapper != nullptr);
    assert(wrapper->socketType == SM_SocketType::Tls);
    
    socketHandler.DestroySocket(wrapper);
    
    std::cout << "✓ TLS socket creation test passed" << std::endl;
}

// Test TLS options
void test_tls_options() {
    std::cout << "Testing TLS options..." << std::endl;
    
    auto* tlsSocket = socketHandler.CreateTLSSocket();
    assert(tlsSocket != nullptr);
    
    // Test boolean options
    assert(tlsSocket->SetOption(SM_SocketOption::TLSVerifyPeer, 1));
    assert(tlsSocket->SetOption(SM_SocketOption::TLSVerifyHost, 1));
    
    // Test version options
    assert(tlsSocket->SetOption(SM_SocketOption::TLSMinVersion, 12)); // TLS 1.2
    assert(tlsSocket->SetOption(SM_SocketOption::TLSMaxVersion, 13)); // TLS 1.3
    
    auto* wrapper = socketHandler.GetSocketWrapper(tlsSocket);
    socketHandler.DestroySocket(wrapper);
    
    std::cout << "✓ TLS options test passed" << std::endl;
}

// Test SSL context
void test_ssl_context() {
    std::cout << "Testing SSL context..." << std::endl;
    
    auto ctx1 = socketHandler.GetSSLContext();
    auto ctx2 = socketHandler.GetSSLContext();
    
    // Should return the same shared context
    assert(ctx1 == ctx2);
    assert(ctx1.use_count() >= 2);
    
    std::cout << "✓ SSL context test passed" << std::endl;
}

// Test enum values
void test_tls_enums() {
    std::cout << "Testing TLS enums..." << std::endl;
    
    // Test socket type enum
    assert(static_cast<int>(SM_SocketType::Tls) == SM_SocketType_Tls);
    assert(SM_SocketType_Tls == 3);
    
    // Test error type enums
    assert(static_cast<int>(SM_ErrorType::TLS_HANDSHAKE_ERROR) == SM_ErrorType_TLS_HANDSHAKE_ERROR);
    assert(static_cast<int>(SM_ErrorType::TLS_CERT_ERROR) == SM_ErrorType_TLS_CERT_ERROR);
    assert(static_cast<int>(SM_ErrorType::TLS_VERSION_ERROR) == SM_ErrorType_TLS_VERSION_ERROR);
    
    std::cout << "✓ TLS enum test passed" << std::endl;
}

int main() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "Running TLS Support Tests" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    try {
        socketHandler.StartProcessing();
        
        test_tls_enums();
        test_ssl_context();
        test_tls_socket_creation();
        test_tls_options();
        
        socketHandler.StopProcessing();
        
        std::cout << "\n========================================" << std::endl;
        std::cout << "All TLS tests passed! ✓" << std::endl;
        std::cout << "========================================" << std::endl;
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}

#else

#include <iostream>

int main() {
    std::cout << "TLS support is not enabled. Build with -DENABLE_TLS=ON" << std::endl;
    return 0;
}

#endif // ENABLE_TLS
