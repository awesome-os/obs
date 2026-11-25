/*
 * Built-in Native Compatibility Layer for Vertical Canvas
 *
 * Maps legacy obs_module_text / obs_module_config_path / obs_module_file calls
 * to the permanent built-in native helpers in vertical-canvas-core.hpp when
 * compiled with OBS_VERTICAL_CANVAS_BUILTIN=1 (zero external plugin DLL).
 *
 * Modified by Awesome-OS (2026-11-25) from Aitum/obs-vertical-canvas (GPL-2.0).
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include "vertical-canvas-core.hpp"

#ifndef OBS_VERTICAL_CANVAS_BUILTIN
#define OBS_VERTICAL_CANVAS_BUILTIN 1
#endif

#if OBS_VERTICAL_CANVAS_BUILTIN
#ifdef obs_module_text
#undef obs_module_text
#endif
#define obs_module_text(lookup_string) obs_vertical_canvas_text(lookup_string)

#ifdef obs_module_config_path
#undef obs_module_config_path
#endif
#define obs_module_config_path(file) obs_vertical_canvas_config_path(file)

#ifdef obs_module_file
#undef obs_module_file
#endif
#define obs_module_file(file) obs_vertical_canvas_data_file(file)
#endif
