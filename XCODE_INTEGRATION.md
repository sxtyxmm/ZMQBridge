# Xcode Integration Guide for ZMQBridge

This guide explains how to integrate the ZMQBridge library into your Xcode project.

## Prerequisites

- macOS with Xcode installed
- Xcode Command Line Tools: `xcode-select --install`
- CMake: `brew install cmake`

## Step 1: Build for macOS

1. **Clone the repository on your Mac:**
   ```bash
   git clone https://github.com/sxtyxmm/ZMQBridge.git
   cd ZMQBridge
   ```

2. **Run the macOS build script:**
   ```bash
   chmod +x build_macos.sh
   ./build_macos.sh
   ```

3. **Verify the build:**
   ```bash
   cd build_macos
   ls -l libprj1*.dylib
   ```

   You should see:
   - `libprj1.dylib` → symlink to libprj1.1.dylib
   - `libprj1.1.dylib` → symlink to libprj1.1.0.0.dylib
   - `libprj1.1.0.0.dylib` → actual library file

## Step 2: Prepare Files for Xcode

Create a directory structure in your Xcode project:

```
YourXcodeProject/
├── External/
│   ├── ZMQBridge/
│   │   ├── lib/
│   │   │   └── libprj1.dylib
│   │   └── include/
│   │       └── prj1.h
```

Copy the files:
```bash
# From the ZMQBridge directory
cp build_macos/libprj1.dylib /path/to/YourXcodeProject/External/ZMQBridge/lib/
cp include/prj1.h /path/to/YourXcodeProject/External/ZMQBridge/include/
```

## Step 3: Configure Xcode Project

### A. Add the Library

1. Open your Xcode project
2. Select your **project** in the navigator
3. Select your **target**
4. Go to **Build Phases** tab
5. Expand **Link Binary With Libraries**
6. Click the **+** button
7. Click **Add Other...** → **Add Files...**
8. Navigate to and select `libprj1.dylib`

### B. Configure Header Search Paths

1. Select your **target** → **Build Settings** tab
2. Search for **"Header Search Paths"**
3. Double-click to edit
4. Add: `$(PROJECT_DIR)/External/ZMQBridge/include`
5. Make sure it's set to **non-recursive**

### C. Configure Library Search Paths

1. Still in **Build Settings**
2. Search for **"Library Search Paths"**
3. Double-click to edit
4. Add: `$(PROJECT_DIR)/External/ZMQBridge/lib`

### D. Configure Runtime Search Paths

1. Still in **Build Settings**
2. Search for **"Runpath Search Paths"**
3. Add the following paths:
   - `@executable_path`
   - `@loader_path`
   - `$(PROJECT_DIR)/External/ZMQBridge/lib`

### E. Disable Bitcode (if needed for older projects)

1. Search for **"Enable Bitcode"**
2. Set to **No**

## Step 4: Use in Your Code

### For Objective-C++ (recommended)

Rename your `.m` file to `.mm` (Objective-C++) and include the header:

```objc
#import <Foundation/Foundation.h>
#include "prj1.h"

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        using namespace prj1;
        
        // Create a client
        ZMQWrapper client;
        Config config;
        config.pattern = Pattern::REQ_REP;
        config.mode = Mode::CLIENT;
        config.timeout_ms = 5000;
        
        if (client.Init(config) != ErrorCode::SUCCESS) {
            NSLog(@"Failed to initialize client");
            return 1;
        }
        
        // Send a message
        client.SendMessage("Hello from Xcode!");
        
        // Receive reply
        std::string reply;
        if (client.ReceiveMessage(reply) == ErrorCode::SUCCESS) {
            NSLog(@"Received: %s", reply.c_str());
        }
        
        client.Close();
    }
    return 0;
}
```

### For Swift (with Bridging Header)

1. **Create a C++ wrapper** (`ZMQBridgeWrapper.h` and `.mm`):

```objc
// ZMQBridgeWrapper.h
#import <Foundation/Foundation.h>

@interface ZMQBridgeWrapper : NSObject

- (BOOL)initializeWithPattern:(NSInteger)pattern 
                         mode:(NSInteger)mode 
                      timeout:(NSInteger)timeout;
- (BOOL)sendMessage:(NSString *)message;
- (NSString *)receiveMessage;
- (void)close;

@end
```

```objc
// ZMQBridgeWrapper.mm
#import "ZMQBridgeWrapper.h"
#include "prj1.h"

@implementation ZMQBridgeWrapper {
    prj1::ZMQWrapper *_wrapper;
}

- (instancetype)init {
    self = [super init];
    if (self) {
        _wrapper = new prj1::ZMQWrapper();
    }
    return self;
}

- (void)dealloc {
    delete _wrapper;
}

- (BOOL)initializeWithPattern:(NSInteger)pattern 
                         mode:(NSInteger)mode 
                      timeout:(NSInteger)timeout {
    prj1::Config config;
    config.pattern = (prj1::Pattern)pattern;
    config.mode = (prj1::Mode)mode;
    config.timeout_ms = (int)timeout;
    
    return _wrapper->Init(config) == prj1::ErrorCode::SUCCESS;
}

- (BOOL)sendMessage:(NSString *)message {
    std::string msg = [message UTF8String];
    return _wrapper->SendMessage(msg) == prj1::ErrorCode::SUCCESS;
}

- (NSString *)receiveMessage {
    std::string msg;
    if (_wrapper->ReceiveMessage(msg) == prj1::ErrorCode::SUCCESS) {
        return [NSString stringWithUTF8String:msg.c_str()];
    }
    return nil;
}

- (void)close {
    _wrapper->Close();
}

@end
```

2. **Use in Swift:**

```swift
import Foundation

let zmq = ZMQBridgeWrapper()
if zmq.initialize(pattern: 0, mode: 1, timeout: 5000) {
    zmq.send("Hello from Swift!")
    if let reply = zmq.receiveMessage() {
        print("Received: \(reply)")
    }
    zmq.close()
}
```

## Step 5: Copy Library at Build Time (Optional but Recommended)

To ensure the .dylib is available at runtime:

1. Go to **Build Phases**
2. Click **+** → **New Copy Files Phase**
3. Set **Destination** to **Executables** (or **Frameworks** for framework)
4. Click **+** and add `libprj1.dylib`

## iOS Integration

For iOS apps, you need to build for iOS architectures:

### Build for iOS

1. **Create iOS build script** (`build_ios.sh`):
   ```bash
   #!/bin/bash
   cmake .. \
       -DCMAKE_SYSTEM_NAME=iOS \
       -DCMAKE_OSX_DEPLOYMENT_TARGET=13.0 \
       -DCMAKE_OSX_ARCHITECTURES="arm64" \
       -DCMAKE_XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH=NO \
       -DBUILD_SHARED_LIBS=OFF
   make -j$(sysctl -n hw.ncpu)
   ```

2. Build as a **static library** instead of shared library
3. Create a Framework bundle for easier integration

## Troubleshooting

### Library not found at runtime

**Error:** `dyld: Library not loaded: libprj1.dylib`

**Solution:** Add to **Runpath Search Paths**:
- `@executable_path`
- `@loader_path`

### Header not found

**Error:** `'prj1.h' file not found`

**Solution:** Verify **Header Search Paths** includes the correct path

### Linking errors

**Error:** `Undefined symbols for architecture x86_64`

**Solution:** 
- Ensure you're using `.mm` extension for Objective-C++
- Verify the library is added to **Link Binary With Libraries**

### Architecture mismatch

**Error:** `building for macOS-arm64 but attempting to link with file built for macOS-x86_64`

**Solution:** Rebuild the library on the same architecture as your Xcode project target

## Testing

Run the test suite on macOS:

```bash
cd build_macos
./prj1_test
```

Test the examples:

```bash
# Terminal 1
./prj1_server

# Terminal 2
./prj1_client
```

## Advanced: Creating a Framework

For cleaner integration, create a proper macOS/iOS Framework:

```bash
# Create framework structure
mkdir -p ZMQBridge.framework/Versions/A/Headers
mkdir -p ZMQBridge.framework/Versions/A/Resources

# Copy files
cp build_macos/libprj1.dylib ZMQBridge.framework/Versions/A/ZMQBridge
cp include/prj1.h ZMQBridge.framework/Versions/A/Headers/

# Create symlinks
cd ZMQBridge.framework
ln -s Versions/A/ZMQBridge ZMQBridge
ln -s Versions/A/Headers Headers
ln -s Versions/A/Resources Resources
```

Then drag the entire `ZMQBridge.framework` into your Xcode project.

## Support

For issues specific to Xcode integration, please open an issue on GitHub with:
- Xcode version
- macOS version
- Error messages
- Build settings screenshots
