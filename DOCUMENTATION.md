# Documentation Generation Guide

This document explains how to generate and access the documentation for SourceMod Socket Extension v2.0.

## Prerequisites

Install Doxygen on your system:

### Ubuntu/Debian
```bash
sudo apt-get install doxygen graphviz
```

### Fedora/RHEL
```bash
sudo dnf install doxygen graphviz
```

### macOS
```bash
brew install doxygen graphviz
```

### Windows
Download from [Doxygen website](https://www.doxygen.nl/download.html)

## Generating Documentation

1. Navigate to the extension directory:
```bash
cd /path/to/sm-ext-socket
```

2. Run Doxygen:
```bash
doxygen Doxyfile
```

3. Documentation will be generated in the `docs/html/` directory

## Viewing Documentation

### Local Viewing
Open `docs/html/index.html` in your web browser:
```bash
# Linux
xdg-open docs/html/index.html

# macOS
open docs/html/index.html

# Windows
start docs/html/index.html
```

## Documentation Structure

The generated documentation includes:

### C++ API Documentation
- **Extension.h** - Main extension interface
- **Socket.h** - Socket wrapper classes
- **SocketTLS.h** - TLS/SSL socket implementation
- **SocketHandler.h** - Socket lifecycle management
- **Callback.h** - Callback system
- **CallbackHandler.h** - Thread-safe callback queue
- **Define.h** - Type definitions and enums

### SourcePawn API Documentation
- **socket.inc** - Native functions, enums, callbacks, and examples

### Guides
- **README.md** - Overview and quick start
- **BUILD.md** - Build instructions
- **TLS_GUIDE.md** - TLS/SSL usage guide
- **TLS_IMPLEMENTATION_SUMMARY.md** - Technical implementation details
- **BUILD_SUMMARY.md** - Build verification and testing

## Online Documentation

If you host the extension on GitHub, you can use GitHub Pages to publish the documentation:

1. Generate documentation as described above
2. Commit the `docs/` directory to your repository
3. Go to Settings → Pages in your GitHub repository
4. Set source to "docs/" folder
5. Your documentation will be available at `https://username.github.io/sm-ext-socket/`

## API Reference Quick Links

### Core Functions
- `SocketCreate()` - Create new socket
- `SocketConnect()` - Connect to remote host
- `SocketSend()` - Send data
- `SocketBind()` - Bind to local address
- `SocketListen()` - Listen for connections

### TLS/SSL Functions
- `SocketConnectTLS()` - Connect with TLS encryption
- `SocketSendTLS()` - Send encrypted data
- `SocketSetTLSOption()` - Configure TLS options
- `SocketSetTLSOptionString()` - Set TLS file paths

### Socket Options
- `SocketSetOption()` - Configure socket behavior
- Performance options: `ConcatenateCallbacks`, `CallbacksPerFrame`
- TLS options: `TLSVerifyPeer`, `TLSMinVersion`, `TLSMaxVersion`
- Low-level options: `SocketSendBuffer`, `SocketReceiveBuffer`

## Examples

Example plugins are available in:
- `examples/tls_example.sp` - TLS/SSL connection example
- `examples/` directory - Additional examples

## Getting Help

If you need help with the documentation:
1. Check the inline comments in header files
2. Review the examples in `examples/` directory
3. Read the guides in the root directory
4. Open an issue on GitHub

## Contributing Documentation

To improve the documentation:
1. Add Doxygen comments to new functions
2. Follow the existing documentation style
3. Update relevant guide documents
4. Regenerate documentation to verify formatting
5. Submit a pull request

## Doxygen Configuration

The documentation is configured via `Doxyfile`:
- **PROJECT_NAME**: "SourceMod Socket Extension"
- **PROJECT_NUMBER**: "2.0.0"
- **OUTPUT_DIRECTORY**: docs
- **EXTRACT_ALL**: YES (documents all functions)
- **SOURCE_BROWSER**: YES (includes source code)
- **GENERATE_TREEVIEW**: YES (hierarchical navigation)

To customize documentation generation, edit `Doxyfile` before running doxygen.
