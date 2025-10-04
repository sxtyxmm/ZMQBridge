#!/bin/bash
# Build script for macOS
# This script should be run on a macOS machine to produce .dylib files for use with Xcode

set -e  # Exit on error

# Color output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}ZMQBridge macOS Build Script${NC}"
echo -e "${GREEN}========================================${NC}"

# Check if running on macOS
if [[ "$OSTYPE" != "darwin"* ]]; then
    echo -e "${RED}Error: This script must be run on macOS${NC}"
    echo "You are running on: $OSTYPE"
    exit 1
fi

# Get the directory where the script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

echo -e "${YELLOW}Current directory: $SCRIPT_DIR${NC}"

# Check for required tools
echo -e "\n${YELLOW}Checking prerequisites...${NC}"

# Check for CMake
if ! command -v cmake &> /dev/null; then
    echo -e "${RED}CMake not found!${NC}"
    echo "Install with: brew install cmake"
    exit 1
else
    echo -e "${GREEN}✓ CMake found:${NC} $(cmake --version | head -1)"
fi

# Check for compiler
if ! command -v clang++ &> /dev/null; then
    echo -e "${RED}clang++ not found!${NC}"
    echo "Install Xcode Command Line Tools with: xcode-select --install"
    exit 1
else
    echo -e "${GREEN}✓ clang++ found:${NC} $(clang++ --version | head -1)"
fi

# Check for make
if ! command -v make &> /dev/null; then
    echo -e "${RED}make not found!${NC}"
    echo "Install Xcode Command Line Tools with: xcode-select --install"
    exit 1
else
    echo -e "${GREEN}✓ make found${NC}"
fi

# Setup ZeroMQ if not already done
if [ ! -d "lib/zeromq/include" ]; then
    echo -e "\n${YELLOW}Setting up ZeroMQ...${NC}"
    if [ -f "setup_zeromq.sh" ]; then
        chmod +x setup_zeromq.sh
        ./setup_zeromq.sh
    else
        echo -e "${RED}Error: setup_zeromq.sh not found${NC}"
        exit 1
    fi
else
    echo -e "${GREEN}✓ ZeroMQ already set up${NC}"
fi

# Create build directory
BUILD_DIR="build_macos"
echo -e "\n${YELLOW}Creating build directory: $BUILD_DIR${NC}"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure with CMake
echo -e "\n${YELLOW}Configuring with CMake...${NC}"
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=10.15 \
    -DBUILD_EXAMPLES=ON \
    -DBUILD_TESTS=ON

# Build
echo -e "\n${YELLOW}Building...${NC}"
CPU_COUNT=$(sysctl -n hw.ncpu)
echo "Using $CPU_COUNT CPU cores"
make -j"$CPU_COUNT"

# Check if build succeeded
if [ $? -eq 0 ]; then
    echo -e "\n${GREEN}========================================${NC}"
    echo -e "${GREEN}Build successful!${NC}"
    echo -e "${GREEN}========================================${NC}"
    
    echo -e "\n${YELLOW}Built artifacts:${NC}"
    ls -lh libprj1*.dylib 2>/dev/null || true
    
    echo -e "\n${YELLOW}Executables:${NC}"
    ls -lh prj1_server prj1_client prj1_test 2>/dev/null || true
    
    echo -e "\n${GREEN}Library details:${NC}"
    if [ -f "libprj1.dylib" ]; then
        file libprj1.dylib
        otool -L libprj1.dylib | head -5
    fi
    
    echo -e "\n${YELLOW}To use in Xcode:${NC}"
    echo "1. Copy these files to your Xcode project:"
    echo "   - $SCRIPT_DIR/$BUILD_DIR/libprj1.dylib"
    echo "   - $SCRIPT_DIR/include/prj1.h"
    echo ""
    echo "2. In Xcode, add libprj1.dylib to:"
    echo "   Target → Build Phases → Link Binary With Libraries"
    echo ""
    echo "3. Add header search path:"
    echo "   Target → Build Settings → Header Search Paths"
    echo "   Add: \$(PROJECT_DIR)/path/to/include"
    echo ""
    echo "4. Add library search path:"
    echo "   Target → Build Settings → Library Search Paths"
    echo "   Add: \$(PROJECT_DIR)/path/to/libs"
    echo ""
    echo -e "${GREEN}Run tests with:${NC} ./prj1_test"
    echo -e "${GREEN}Try examples:${NC} ./prj1_server & ./prj1_client"
else
    echo -e "\n${RED}========================================${NC}"
    echo -e "${RED}Build failed!${NC}"
    echo -e "${RED}========================================${NC}"
    exit 1
fi
