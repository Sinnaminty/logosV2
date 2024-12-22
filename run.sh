#!/bin/bash

# Check if the build directory exists
if [ -d "build" ]; then
  echo "Removing existing build directory..."
  rm -rf build
fi

# Generate the build system
echo "Running cmake to configure the build..."
cmake -B build

# Build the project
echo "Building the project..."
cmake --build build

# Run the executable
echo "Running the program..."

./build/Logos
