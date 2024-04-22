
# Simple Asio Web Server
Simple web server implementation using Boost Asio.
# Requirements
* git
* cmake
* make
* Boost Asio
# Build
```bash
cmake -S . -B build
cd build
make
```
Executable will appear in the **bin** directory.
# Usage
By default, the server runs on port *8888*, but this can be changed in the *main.cpp* file.
Simply run the program and paste the shared files into the **public** folder.