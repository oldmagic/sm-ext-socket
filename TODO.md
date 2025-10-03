# TODO List for Socket Extension v2.0

This document tracks the remaining work items for completing the modernization.

## 🔴 Critical - Must Complete for v2.0

### Implementation Files Update
- [ ] **Socket.cpp** - Rewrite using modern patterns
  - [ ] Replace raw pointers with std::unique_ptr
  - [ ] Use std::mutex instead of boost::mutex
  - [ ] Replace boost::bind with lambdas
  - [ ] Update for std::error_code
  - [ ] Test template instantiation

- [ ] **SocketHandler.cpp** - Modernize resource management
  - [ ] Update for std::unique_ptr<io_service>
  - [ ] Replace boost::thread with std::thread
  - [ ] Update CreateSocket to return proper types
  - [ ] Fix destructor for smart pointer cleanup

- [ ] **Callback.cpp** - Implement std::variant-based storage
  - [ ] Refactor constructors for std::variant
  - [ ] Update ExecuteHelper for modern types
  - [ ] Remove manual memory management
  - [ ] Test all callback types

- [ ] **CallbackHandler.cpp** - Thread-safe queue operations
  - [ ] Update for std::unique_ptr<Callback>
  - [ ] Replace boost::mutex with std::mutex
  - [ ] Test thread safety

- [ ] **Extension.cpp** - Bridge to SourceMod
  - [ ] Update for modernized Socket types
  - [ ] Ensure backward compatibility
  - [ ] Test all natives
  - [ ] Update version to 2.0.0

### IPv6 Support Implementation
- [ ] Add IPv6 resolver support
- [ ] Implement dual-stack socket binding
- [ ] Add SocketBindIPv6 native
- [ ] Add SocketConnectIPv6 native
- [ ] Add SocketListenDualStack native
- [ ] Test IPv6-only connections
- [ ] Test IPv4-IPv6 dual-stack
- [ ] Test IPv4-mapped IPv6 addresses

### TLS/SSL Support Implementation
- [ ] Integrate Asio SSL stream wrapper
- [ ] Add OpenSSL context management
- [ ] Implement SocketConnectTLS native
- [ ] Add certificate verification options
- [ ] Add TLS socket options
- [ ] Test TLS 1.2 connections
- [ ] Test TLS 1.3 connections
- [ ] Test certificate validation
- [ ] Test self-signed certificates

### SourcePawn API Updates
- [ ] Update socket.inc with new natives
- [ ] Add doc comments to all natives
- [ ] Add TLS-related enums
- [ ] Add IPv6-related enums
- [ ] Add error code enums
- [ ] Test backward compatibility

## 🟡 Important - Should Complete for v2.0

### Testing
- [ ] Write Socket class unit tests
  - [ ] Test Connect/Disconnect
  - [ ] Test Send/Receive
  - [ ] Test Bind/Listen
  - [ ] Test error handling
  
- [ ] Write SocketHandler unit tests
  - [ ] Test socket creation
  - [ ] Test socket destruction
  - [ ] Test I/O service lifecycle
  
- [ ] Write Callback system unit tests
  - [ ] Test callback creation
  - [ ] Test callback execution
  - [ ] Test thread safety
  
- [ ] Write integration tests
  - [ ] TCP echo test
  - [ ] UDP datagram test
  - [ ] TLS handshake test
  - [ ] IPv6 connectivity test
  - [ ] Error scenario tests

### Documentation
- [ ] Generate Doxygen HTML docs
- [ ] Update socket.inc with complete docs
- [ ] Add code comments to complex functions
- [ ] Create troubleshooting guide
- [ ] Add performance tuning guide

### Build System
- [ ] Test CMake on Ubuntu 20.04
- [ ] Test CMake on Ubuntu 22.04
- [ ] Test CMake on Debian 11
- [ ] Test CMake on Windows 10/11
- [ ] Test with GCC 9, 10, 11
- [ ] Test with Clang 10, 11, 12
- [ ] Test with MSVC 2019
- [ ] Test with MSVC 2022
- [ ] Verify 32-bit builds
- [ ] Verify 64-bit builds

## 🟢 Nice to Have - Can Defer to v2.1+

### Advanced Features
- [ ] C++20 coroutine support
- [ ] std::future-based async API
- [ ] Connection pooling utilities
- [ ] HTTP client helper functions
- [ ] WebSocket support
- [ ] SOCKS proxy support
- [ ] DNS caching
- [ ] Rate limiting utilities

### Performance Optimization
- [ ] Benchmark vs v1.x
- [ ] Profile with Valgrind
- [ ] Profile with perf
- [ ] Optimize hot paths
- [ ] Zero-copy operations
- [ ] Lock-free queue (if beneficial)

### Additional Testing
- [ ] Stress testing
- [ ] Load testing
- [ ] Fuzzing
- [ ] Memory leak detection
- [ ] Thread sanitizer
- [ ] Address sanitizer
- [ ] Coverage analysis

### Enhanced Documentation
- [ ] Video tutorials
- [ ] Interactive examples
- [ ] API reference website
- [ ] Performance comparison charts
- [ ] Architecture diagrams

## 📋 Checklist for Each Implementation File

When updating .cpp files, ensure:

### Code Quality
- [ ] All raw pointers replaced with smart pointers
- [ ] All boost::mutex replaced with std::mutex
- [ ] All boost::bind replaced with lambdas
- [ ] All boost::thread replaced with std::thread
- [ ] No manual new/delete
- [ ] Proper const-correctness
- [ ] Exception safety
- [ ] Thread safety

### Documentation
- [ ] Doxygen comments on public functions
- [ ] Complex algorithms explained
- [ ] Thread-safety documented
- [ ] Error conditions documented

### Testing
- [ ] Unit tests written
- [ ] Edge cases tested
- [ ] Error paths tested
- [ ] Thread safety tested

### Performance
- [ ] No unnecessary allocations
- [ ] Move semantics used where appropriate
- [ ] Efficient algorithms
- [ ] No lock contention

## 🎯 Milestones

### Milestone 1: Headers Complete ✅
**Status:** Done
- All header files modernized
- Smart pointers, modern threading
- Type-safe enums

### Milestone 2: Core Implementation
**Status:** In Progress (0%)
**Target:** Week 1-2
- Socket.cpp updated
- SocketHandler.cpp updated
- Callback.cpp updated
- Extension.cpp updated

### Milestone 3: IPv6 Support
**Status:** Not Started
**Target:** Week 3
- IPv6 natives implemented
- Dual-stack support
- Tests passing

### Milestone 4: TLS/SSL Support
**Status:** Not Started
**Target:** Week 4
- TLS natives implemented
- Certificate handling
- Tests passing

### Milestone 5: Testing Complete
**Status:** Not Started
**Target:** Week 5-6
- Unit tests comprehensive
- Integration tests passing
- Performance validated

### Milestone 6: Release Ready
**Status:** Not Started
**Target:** Week 7-8
- All platforms building
- Documentation complete
- Alpha/Beta testing done
- v2.0.0 released

## 📝 Notes

### Backward Compatibility Strategy
- Keep old enum values as constexpr
- Maintain SourcePawn API unchanged
- Extension should be drop-in replacement
- Test with existing v1.x plugins

### Performance Targets
- Connection latency: < 2ms (vs 5ms in v1.x)
- Throughput: > 1.2 GB/s (vs 800 MB/s in v1.x)
- Memory: < 2 KB per socket (vs 4 KB in v1.x)
- CPU: -30% vs v1.x

### Testing Strategy
1. Unit tests for each class
2. Integration tests for workflows
3. Performance benchmarks
4. Real-world game server testing
5. Community alpha/beta testing

### Release Criteria
- [ ] All critical tasks complete
- [ ] All tests passing
- [ ] No known critical bugs
- [ ] Performance targets met
- [ ] Documentation complete
- [ ] Builds on all platforms
- [ ] Backward compatible

## 🐛 Known Issues to Address

### From Code Review
- [ ] Template instantiation warnings
- [ ] Potential race condition in callback queue
- [ ] Error handling needs improvement
- [ ] Some legacy code patterns remain

### From Testing
- [ ] (None yet - tests not written)

## 💡 Ideas for Future Versions

### v2.1
- HTTP/HTTPS client helpers
- Connection pooling
- Better error messages
- Performance dashboard

### v2.2
- WebSocket support
- HTTP/2 support
- QUIC protocol support
- Zero-copy optimizations

### v3.0
- Full C++20 adoption
- Coroutine-based API
- Async/await style
- Breaking API changes allowed

## 📞 Getting Help

If stuck on any task:
1. Check existing code for patterns
2. Review C++17 references
3. Check Asio documentation
4. Ask in GitHub Discussions
5. Review SourceMod SDK docs

## 🎓 Learning Resources

- [C++17 Features](https://en.cppreference.com/w/cpp/17)
- [Asio Documentation](https://think-async.com/Asio/)
- [Modern C++ Best Practices](https://github.com/cpp-best-practices)
- [SourceMod SDK](https://wiki.alliedmods.net/SourceMod_SDK)
- [CMake Tutorial](https://cmake.org/cmake/help/latest/guide/tutorial/)

---

**Last Updated:** 2024
**Next Review:** After each milestone
