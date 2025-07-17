#!/bin/bash
# Qt Environment Setup Script

export QT_DIR="/Users/yhx/Qt/6.9.1/macos"
export PATH="/Users/yhx/Qt/Tools/CMake/CMake.app/Contents/bin:/Users/yhx/Qt/6.9.1/macos/bin:$PATH"
export CMAKE_PREFIX_PATH="/Users/yhx/Qt/6.9.1/macos"

echo "Qt environment configured:"
echo "QT_DIR: $QT_DIR"
echo "CMAKE_PREFIX_PATH: $CMAKE_PREFIX_PATH"
echo "PATH updated with Qt tools"
