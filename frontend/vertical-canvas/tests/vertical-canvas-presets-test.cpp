/*
 * Standalone C++17 Smoke & Unit Test for Built-in Vertical Canvas Core API
 * and Multi-Stream Presets (Twitch + TikTok + YouTube Shorts + Reels + Kick).
 *
 * Compile natively with Zig (zero WSL):
 *   .\bin\zig.cmd c++ -std=c++17 -O2 -Wall -Wextra -Werror \
 *     frontend/vertical-canvas/multistream-presets.cpp \
 *     frontend/vertical-canvas/tests/vertical-canvas-presets-test.cpp \
 *     -I frontend/vertical-canvas -o vertical-canvas-presets-test.exe
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "multistream-presets.hpp"
#include "vertical-canvas-core.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

#define CHECK(cond)                                                                                \
	do {                                                                                       \
		if (!(cond)) {                                                                     \
			std::cerr << "[FAIL] " << __FILE__ << ":" << __LINE__ << ": " #cond "\n";  \
			std::exit(1);                                                              \
		}                                                                                  \
	} while (0)

/* Stub native C lifecycle symbols for standalone header linkage verification */
namespace {
int g_init_calls = 0;
int g_post_init_calls = 0;
int g_shutdown_calls = 0;
} // namespace

extern "C" bool obs_vertical_canvas_init(void)
{
	++g_init_calls;
	return true;
}

extern "C" void obs_vertical_canvas_post_init(void)
{
	++g_post_init_calls;
}

extern "C" void obs_vertical_canvas_shutdown(void)
{
	++g_shutdown_calls;
}

extern "C" const char *obs_vertical_canvas_text(const char *lookup_string)
{
	return lookup_string ? lookup_string : "";
}

extern "C" char *obs_vertical_canvas_config_path(const char *)
{
	return nullptr;
}

extern "C" char *obs_vertical_canvas_data_file(const char *)
{
	return nullptr;
}

int main()
{
	using namespace awesome_obs;

	/* 1. Verify native built-in lifecycle RAII guard & static facade */
	{
		VerticalCanvasFrontendGuard guard;
		CHECK(guard.IsInitialized());
		guard.PostInitialize();
		CHECK(g_init_calls == 1);
		CHECK(g_post_init_calls == 1);
		CHECK(g_shutdown_calls == 0);
	}
	CHECK(g_shutdown_calls == 1);

	/* 2. Verify all 5 built-in stream targets exist and pass validation */
	const auto all = MultistreamPresets::AllTargets();
	CHECK(all.size() == 5);
	for (const auto &target : all) {
		CHECK(MultistreamPresets::ValidatePreset(target));
	}

	/* 3. Verify Twitch Horizontal 1920x1080 @ 60fps */
	const auto twitch = MultistreamPresets::FindById("twitch-horizontal-1080p60");
	CHECK(twitch.has_value());
	CHECK(twitch->orientation == CanvasOrientation::Horizontal);
	CHECK(twitch->width == 1920 && twitch->height == 1080 && twitch->fps == 60);
	CHECK(twitch->video_bitrate_kbps == 6000);

	/* 4. Verify TikTok Live Vertical 1080x1920 @ 60fps */
	const auto tiktok = MultistreamPresets::FindById("tiktok-vertical-1080p60");
	CHECK(tiktok.has_value());
	CHECK(tiktok->orientation == CanvasOrientation::Vertical);
	CHECK(tiktok->width == 1080 && tiktok->height == 1920 && tiktok->fps == 60);
	CHECK(tiktok->requires_ephemeral_stream_key);

	/* 5. Verify YouTube Shorts Vertical 1080x1920 @ 60fps RTMPS */
	const auto yt = MultistreamPresets::FindById("youtube-shorts-1080p60");
	CHECK(yt.has_value());
	CHECK(yt->protocol == StreamProtocol::RTMPS);
	CHECK(yt->ingest_url.find("rtmps://a.rtmps.youtube.com/live2") == 0);
	CHECK(yt->width == 1080 && yt->height == 1920 && yt->fps == 60);

	/* 6. Verify Instagram Reels (30fps) & Kick Vertical (60fps) */
	const auto reels = MultistreamPresets::FindById("instagram-reels-1080p30");
	CHECK(reels.has_value());
	CHECK(reels->width == 1080 && reels->height == 1920 && reels->fps == 30);

	const auto kick = MultistreamPresets::FindById("kick-vertical-1080p60");
	CHECK(kick.has_value());
	CHECK(kick->width == 1080 && kick->height == 1920 && kick->fps == 60);

	/* 7. Verify DefaultBundle has Linked Scene Switching, Shared Encoder & Backtrack ON */
	const auto bundle = MultistreamPresets::DefaultBundle();
	CHECK(bundle.horizontal_primary.id == "twitch-horizontal-1080p60");
	CHECK(bundle.vertical_targets.size() == 4);
	CHECK(bundle.linked_scene_switching);
	CHECK(bundle.shared_encoder_with_main);
	CHECK(bundle.streaming_match_main);
	CHECK(bundle.recording_match_main);
	CHECK(bundle.backtrack_enabled);
	CHECK(bundle.backtrack_always_on);
	CHECK(bundle.backtrack_duration_sec == 60);

	/* 8. Verify JSON serialization for first-boot config.json */
	const std::string json = MultistreamPresets::SerializeVerticalConfigJson(bundle);
	CHECK(json.find("\"width\": 1080") != std::string::npos);
	CHECK(json.find("\"height\": 1920") != std::string::npos);
	CHECK(json.find("\"linked_scene_switching\": true") != std::string::npos);
	CHECK(json.find("\"stream_advanced_settings\": false") != std::string::npos);
	CHECK(json.find("\"backtrack\": true") != std::string::npos);
	CHECK(json.find("rtmps://a.rtmps.youtube.com/live2") != std::string::npos);
	CHECK(json.find("TikTok Live Vertical") != std::string::npos);
	CHECK(json.find("Kick Vertical") != std::string::npos);

	/* 9. Verify negative validation catches odd / mismatched dimensions */
	StreamTargetPreset bad = *tiktok;
	bad.width = 1081;
	CHECK(!MultistreamPresets::ValidatePreset(bad));
	bad.width = 1920;
	bad.height = 1080; /* horizontal dims with Vertical orientation */
	CHECK(!MultistreamPresets::ValidatePreset(bad));

	std::cout << "[PASS] Built-in Vertical Canvas Core API & Multi-Stream Presets ("
		  << all.size() << " targets, " << bundle.vertical_targets.size()
		  << " vertical presets, Linked Scene Switching + Shared Encoder + Backtrack ON)\n";
	return 0;
}
