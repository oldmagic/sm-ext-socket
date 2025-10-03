SourceMod Socket Extension
==========================
This has been forked from [sfPlayer's origin Git repository](http://player.to/gitweb/index.cgi?p=sm-ext-socket.git). Get to the [AlliedModders forum thread](https://forums.alliedmods.net/showthread.php?t=67640) for more information.

## Features
- TCP and UDP socket support
- Asynchronous I/O operations using Boost.Asio
- Thread-safe callback management
- Connection-oriented and connectionless protocols
- Server socket support (listening/accepting)
- Modern SourcePawn API

## Dependencies for Building
 * [SourceMod](https://github.com/alliedmodders/sourcemod) (>= 1.5.x)
 * [Boost](http://www.boost.org/) (>= 1.55)
 * [MetaMod:Source](https://github.com/alliedmodders/metamod-source)
 * [msinttypes](https://code.google.com/p/msinttypes/) (Windows only)

## Building on Windows
Set the following environment variables on your system:
 * `SOURCEMOD` - path to SourceMod
 * `BOOST155` - path to Boost libraries
 * `MSINTTYPES` - path to latest version of msinttypes

## Building on Linux
Edit the Makefile to set the correct paths for:
 * `SMSDK` - path to SourceMod SDK
 * `SOURCEMM` - path to MetaMod:Source

Then run:
```bash
make
```

## Usage
See the `examples/` directory for usage examples. The extension provides natives for:
- Creating TCP/UDP sockets
- Connecting to remote hosts
- Sending and receiving data
- Listening for incoming connections
- Setting socket options

All examples have been updated to use modern SourcePawn syntax compatible with SourceMod 1.7+.

## License
This extension is licensed under the GNU General Public License v3. See LICENSE for details.
