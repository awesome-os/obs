# Built-in Native Vertical Canvas for Awesome-OS OBS Studio
# Hard-forked from Aitum/obs-vertical-canvas (GPL-2.0) and converted from an
# external loadable plugin (MODULE) into a permanent STATIC frontend library.

cmake_minimum_required(VERSION 3.22)

project(obs-vertical-canvas VERSION 1.6.8 LANGUAGES C CXX)

option(OBS_VERTICAL_CANVAS_BUILD_TESTS "Build standalone C++17 presets smoke tests" ON)
option(OBS_VERTICAL_CANVAS_BUILD_FRONTEND "Build Qt6/libobs static frontend library" OFF)

if(CMAKE_PROJECT_NAME STREQUAL "obs-studio" OR TARGET OBS::libobs)
  set(OBS_VERTICAL_CANVAS_BUILD_FRONTEND ON)
endif()

# Pure C++17 Multi-Stream Presets & Crash-Proof Dock Guardian library (zero Qt/libobs dependencies)
add_library(obs-vertical-canvas-presets STATIC
  multistream-presets.cpp
  multistream-presets.hpp
  dock-session-guardian.cpp
  dock-session-guardian.hpp
)
target_compile_features(obs-vertical-canvas-presets PUBLIC cxx_std_17)
target_include_directories(obs-vertical-canvas-presets PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

if(OBS_VERTICAL_CANVAS_BUILD_FRONTEND)
  find_package(Qt6 REQUIRED COMPONENTS Widgets Core)
  if(UNIX AND NOT APPLE AND Qt6_VERSION VERSION_LESS "6.9.0")
    find_package(Qt6 COMPONENTS GuiPrivate)
  endif()

  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/version.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/version.h"
  )

  # Permanent built-in STATIC library (NOT MODULE / NOT an external plugin DLL)
  add_library(obs-vertical-canvas STATIC
    vertical-canvas.cpp
    scenes-dock.cpp
    sources-dock.cpp
    source-tree.cpp
    transitions-dock.cpp
    qt-display.cpp
    projector.cpp
    config-dialog.cpp
    hotkey-edit.cpp
    name-dialog.cpp
    audio-wrapper-source.c
    multi-canvas-source.c
    resources.qrc
    vertical-canvas-core.hpp
    vertical-canvas-builtin-compat.hpp
    vertical-canvas.hpp
    scenes-dock.hpp
    sources-dock.hpp
    source-tree.hpp
    transitions-dock.hpp
    qt-display.hpp
    projector.hpp
    display-helpers.hpp
    config-dialog.hpp
    hotkey-edit.hpp
    name-dialog.hpp
    audio-wrapper-source.h
    obs-websocket-api.h
    multi-canvas-source.h
  )

  target_compile_definitions(obs-vertical-canvas PUBLIC OBS_VERTICAL_CANVAS_BUILTIN=1)
  target_include_directories(obs-vertical-canvas PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_BINARY_DIR}"
  )

  target_link_libraries(obs-vertical-canvas
    PUBLIC
      obs-vertical-canvas-presets
    PRIVATE
      OBS::libobs
      OBS::frontend-api
      Qt6::Core
      Qt6::Widgets
  )

  if(TARGET Qt6::GuiPrivate)
    target_link_libraries(obs-vertical-canvas PRIVATE Qt6::GuiPrivate)
  endif()

  set_target_properties(obs-vertical-canvas PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
  )

  # Link directly into the OBS Studio frontend target when built in-tree
  if(TARGET obs-studio)
    target_link_libraries(obs-studio PRIVATE obs-vertical-canvas)
  elseif(TARGET obs)
    target_link_libraries(obs PRIVATE obs-vertical-canvas)
  endif()
endif()

if(OBS_VERTICAL_CANVAS_BUILD_TESTS)
  enable_testing()
  add_executable(vertical-canvas-presets-test
    tests/vertical-canvas-presets-test.cpp
  )
  target_link_libraries(vertical-canvas-presets-test PRIVATE obs-vertical-canvas-presets)
  add_test(NAME vertical-canvas-presets-test COMMAND vertical-canvas-presets-test)

  add_executable(dock-guardian-test
    tests/dock-guardian-test.cpp
  )
  target_link_libraries(dock-guardian-test PRIVATE obs-vertical-canvas-presets)
  add_test(NAME dock-guardian-test COMMAND dock-guardian-test)
endif()
