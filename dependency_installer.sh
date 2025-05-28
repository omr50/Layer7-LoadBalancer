#!/bin/bash

# Update package lists and install dependencies
sudo apt-get update
sudo apt-get install -y build-essential git autoconf automake libtool

# Clone the http-parser repository from GitHub
git clone https://github.com/nodejs/http-parser.git

# Navigate to the cloned directory
cd http-parser

# Generate configuration files using autogen
./autogen.sh

# Build the project
make

# Install the library
sudo make install

# Clean up
cd ..
rm -rf http-parser

echo "http-parser installation complete!"

