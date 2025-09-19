# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview
libfvde is a library for accessing FileVault Drive Encryption (FVDE/FileVault2) encrypted volumes used by macOS. This is a C library with Python bindings and command-line tools for digital forensics and security analysis.

## Build System and Common Commands

### Initial Setup
```bash
# Generate configure script
./autogen.sh

# Standard build
./configure
make

# Build specific components
make library    # Build just the library components
```

### Testing
```bash
# Run all tests
./runtests.sh

# Run specific test suites
cd tests && ./test_library.sh      # Library tests
cd tests && ./test_tools.sh        # Tools tests  
cd tests && ./test_python_module.sh # Python binding tests
```

### Development Workflows
The project uses GNU autotools for building. After modifying source files:
1. Run `make` to rebuild
2. Use `./runtests.sh` to verify changes
3. The build system automatically handles dependencies between library components

## Architecture

### Core Library Structure
- **libfvde/**: Main library implementation in C
  - Volume handling, metadata parsing, encryption/decryption
  - Segment descriptor system for tracking FVDE snapshots
  - Dependencies on multiple supporting libraries (libcaes, libhmac, etc.)

- **Supporting Libraries**: Multiple lib* directories provide modular functionality:
  - `libcaes`: AES encryption support
  - `libhmac`: HMAC cryptographic functions  
  - `libfdata`: Data management and caching
  - `libfplist`: Property list parsing
  - Plus ~15 other supporting libraries

### Tools and Bindings
- **fvdetools/**: Command-line utilities for working with FVDE volumes
- **pyfvde/**: Python bindings for the library
- **tests/**: Comprehensive test suite with both C and Python tests

### Key Components
- **Volume Group Management**: Handles Core Storage logical/physical volume relationships
- **Metadata Processing**: Parses encrypted metadata blocks and volume headers
- **Encryption Context**: Manages encryption keys and contexts from plists
- **Segment Tracking**: New experimental feature for handling FVDE snapshot nature

## Development Notes

### Code Conventions
- Standard C89/C90 code style
- Consistent error handling patterns using libcerror
- Memory management follows library-specific patterns
- Function naming: `libfvde_[component]_[action]` format

### Testing Strategy
- Unit tests for individual components in tests/ directory
- Integration tests via shell scripts
- Python module tests for binding functionality
- OSS-Fuzz integration for security testing

### Dependencies
The project has a complex dependency tree of supporting libraries. Most dependencies can be built from included subdirectories or linked against system versions via configure options.