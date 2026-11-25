# Awesome-OS OBS Studio — Root CMake Integration
# Wires the permanent built-in native Vertical Canvas static library into the
# OBS frontend (zero external plugin DLL).

cmake_minimum_required(VERSION 3.22)
project(awesome-os-obs VERSION 0.2.0 LANGUAGES C CXX)

add_subdirectory(frontend/vertical-canvas)
