#!/bin/bash
# Script to download and setup ZeroMQ library

set -e

ZEROMQ_VERSION="4.3.5"
ZEROMQ_DIR="lib/zeromq"
DOWNLOAD_URL="https://github.com/zeromq/libzmq/releases/download/v${ZEROMQ_VERSION}/zeromq-${ZEROMQ_VERSION}.tar.gz"

echo "Downloading ZeroMQ ${ZEROMQ_VERSION}..."

# Create lib directory if it doesn't exist
mkdir -p lib

# Download ZeroMQ
cd lib
if [ ! -f "zeromq-${ZEROMQ_VERSION}.tar.gz" ]; then
    wget -q --show-progress "${DOWNLOAD_URL}" || curl -L -o "zeromq-${ZEROMQ_VERSION}.tar.gz" "${DOWNLOAD_URL}"
fi

echo "Extracting ZeroMQ..."
tar -xzf "zeromq-${ZEROMQ_VERSION}.tar.gz"

# Remove old zeromq directory if exists
rm -rf zeromq

# Rename to zeromq
mv "zeromq-${ZEROMQ_VERSION}" zeromq

echo "ZeroMQ ${ZEROMQ_VERSION} has been set up in ${ZEROMQ_DIR}"
echo "You can now build the project with CMake"
