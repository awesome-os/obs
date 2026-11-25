/*
 * Built-in Native Vertical Canvas Core API for Awesome-OS OBS Studio
 *
 * Hard-forked from Aitum/obs-vertical-canvas (Copyright (C) Aitum / Exeldro)
 * Converted from an external OBS plugin into a permanent built-in native
 * OBS frontend module (zero external plugin DLL).
 *
 * Modified by Awesome-OS (2026-11-25): Replaced external obs_module_* plugin
 * lifecycle with native frontend static-linked entrypoints and added built-in
 * Multi-Stream presets.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize the built-in Vertical Canvas core subsystem (registers
 * audio_wrapper_source, multi_canvas_source, proc handlers, and frontend
 * event callbacks). Call during OBS frontend initialization (OBSBasic / obs-app).
 */
bool obs_vertical_canvas_init(void);

/**
 * Complete post-load Vertical Canvas dock creation and obs-websocket vendor
 * registration once the OBS main window and plugins are ready.
 */
void obs_vertical_canvas_post_init(void);

/**
 * Shut down the built-in Vertical Canvas core subsystem, unregistering
 * callbacks and vendor requests. Call during OBS frontend teardown.
 */
void obs_vertical_canvas_shutdown(void);

/**
 * Built-in localization lookup for Vertical Canvas UI strings (replaces
 * external obs_module_text() without requiring an obs_module_t).
 */
const char *obs_vertical_canvas_text(const char *lookup_string);

/**
 * Resolve a config file path inside the user's OBS configuration directory
 * (caller must free with bfree(); replaces obs_module_config_path()).
 */
char *obs_vertical_canvas_config_path(const char *file);

/**
 * Resolve a bundled data asset path for the built-in Vertical Canvas
 * (caller must free with bfree(); replaces obs_module_file()).
 */
char *obs_vertical_canvas_data_file(const char *file);

#ifdef __cplusplus
}

namespace awesome_obs {

/**
 * C++ RAII / static facade for OBS frontend integration (OBSBasic / obs-app.cpp).
 */
class VerticalCanvasCore {
public:
	static bool Initialize() { return obs_vertical_canvas_init(); }
	static void PostInitialize() { obs_vertical_canvas_post_init(); }
	static void Shutdown() { obs_vertical_canvas_shutdown(); }
};

/**
 * Scoped RAII guard for OBSBasic / obs-app.cpp startup and teardown.
 */
class VerticalCanvasFrontendGuard {
public:
	VerticalCanvasFrontendGuard() : initialized_(VerticalCanvasCore::Initialize()) {}
	~VerticalCanvasFrontendGuard()
	{
		if (initialized_) {
			VerticalCanvasCore::Shutdown();
		}
	}

	VerticalCanvasFrontendGuard(const VerticalCanvasFrontendGuard &) = delete;
	VerticalCanvasFrontendGuard &operator=(const VerticalCanvasFrontendGuard &) = delete;

	bool IsInitialized() const noexcept { return initialized_; }
	void PostInitialize() const
	{
		if (initialized_) {
			VerticalCanvasCore::PostInitialize();
		}
	}

private:
	bool initialized_ = false;
};

} // namespace awesome_obs
#endif
