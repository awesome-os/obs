/*
 * Built-in Multi-Stream Presets for Awesome-OS OBS Vertical Canvas
 *
 * Provides 1-click default presets for simultaneous Horizontal + Vertical
 * multistreaming across Twitch, TikTok Live, YouTube Shorts, Instagram Reels,
 * and Kick, with Linked Scene Switching, Shared Encoder, and Backtrack Replay
 * Buffer enabled out of the box.
 *
 * Copyright (C) 2026 Awesome-OS Contributors
 * Built on Aitum/obs-vertical-canvas (Copyright (C) Aitum / Exeldro, GPL-2.0).
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace awesome_obs {

enum class CanvasOrientation : uint8_t {
	Horizontal = 0,
	Vertical = 1,
};

enum class StreamProtocol : uint8_t {
	RTMP = 0,
	RTMPS = 1,
	WHIP = 2,
};

struct StreamTargetPreset {
	std::string id;
	std::string display_name;
	std::string platform;
	CanvasOrientation orientation = CanvasOrientation::Vertical;
	StreamProtocol protocol = StreamProtocol::RTMPS;
	std::string ingest_url;
	uint32_t width = 1080;
	uint32_t height = 1920;
	uint32_t fps = 60;
	uint32_t video_bitrate_kbps = 6000;
	uint32_t audio_bitrate_kbps = 160;
	uint32_t keyframe_interval_sec = 2;
	bool enabled_by_default = false;
	bool requires_ephemeral_stream_key = false;
};

struct MultistreamBundle {
	std::string id;
	std::string name;
	std::string description;
	StreamTargetPreset horizontal_primary;
	std::vector<StreamTargetPreset> vertical_targets;

	/* Zero-config creator defaults */
	bool linked_scene_switching = true;
	bool shared_encoder_with_main = true;
	bool streaming_match_main = true;
	bool recording_match_main = true;
	bool backtrack_enabled = true;
	bool backtrack_always_on = true;
	uint32_t backtrack_duration_sec = 60;
};

class MultistreamPresets {
public:
	/**
	 * Built-in catalog of individual 1-click horizontal & vertical presets:
	 *  - twitch-horizontal-1080p60   (1920x1080 @ 60fps, 6000 kbps)
	 *  - tiktok-vertical-1080p60     (1080x1920 @ 60fps, 6000 kbps)
	 *  - youtube-shorts-1080p60      (1080x1920 @ 60fps, RTMPS, 6000 kbps)
	 *  - instagram-reels-1080p30     (1080x1920 @ 30fps, RTMPS, 4500 kbps)
	 *  - kick-vertical-1080p60       (1080x1920 @ 60fps, RTMPS, 6000 kbps)
	 */
	static std::vector<StreamTargetPreset> AllTargets();

	/**
	 * Built-in 1-click default multistream bundle:
	 * Twitch Horizontal (1920x1080 @ 60fps) + TikTok Live / YouTube Shorts /
	 * Instagram Reels / Kick Vertical (1080x1920) with Linked Scene Switching,
	 * Shared Encoder, and Backtrack Replay Buffer enabled by default.
	 */
	static MultistreamBundle DefaultBundle();

	/**
	 * Lookup a single target preset by its stable identifier.
	 */
	static std::optional<StreamTargetPreset> FindById(std::string_view id);

	/**
	 * Return only vertical (9:16) stream target presets.
	 */
	static std::vector<StreamTargetPreset> VerticalTargets();

	/**
	 * Validate preset dimensions (even, non-zero), orientation aspect ratio,
	 * bitrate bounds, and non-empty ingest URL.
	 */
	static bool ValidatePreset(const StreamTargetPreset &preset);

	/**
	 * Serialize a MultistreamBundle into a Vertical Canvas config.json payload
	 * compatible with CanvasDock::CanvasDock() first-boot initialization.
	 */
	static std::string SerializeVerticalConfigJson(const MultistreamBundle &bundle);
};

} // namespace awesome_obs
