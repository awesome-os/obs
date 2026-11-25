/*
 * Built-in Multi-Stream Presets Implementation for Awesome-OS OBS Vertical Canvas
 *
 * Copyright (C) 2026 Awesome-OS Contributors
 * Built on Aitum/obs-vertical-canvas (Copyright (C) Aitum / Exeldro, GPL-2.0).
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "multistream-presets.hpp"

#include <sstream>

namespace awesome_obs {

namespace {

std::string EscapeJsonString(std::string_view input)
{
	std::string out;
	out.reserve(input.size() + 8);
	for (char c : input) {
		switch (c) {
		case '"':
			out += "\\\"";
			break;
		case '\\':
			out += "\\\\";
			break;
		case '\n':
			out += "\\n";
			break;
		case '\r':
			out += "\\r";
			break;
		case '\t':
			out += "\\t";
			break;
		default:
			out.push_back(c);
			break;
		}
	}
	return out;
}

} // namespace

std::vector<StreamTargetPreset> MultistreamPresets::AllTargets()
{
	return {
		StreamTargetPreset{
			"twitch-horizontal-1080p60",
			"Twitch Horizontal (1080p60)",
			"Twitch",
			CanvasOrientation::Horizontal,
			StreamProtocol::RTMPS,
			"rtmps://ingest.global-contribute.live-video.net/app/",
			1920,
			1080,
			60,
			6000,
			160,
			2,
			true,
			false,
		},
		StreamTargetPreset{
			"tiktok-vertical-1080p60",
			"TikTok Live Vertical (1080x1920 @ 60fps)",
			"TikTok Live",
			CanvasOrientation::Vertical,
			StreamProtocol::RTMPS,
			"rtmps://push-rtmp-f5-tt.tiktokcdn.com/stage/",
			1080,
			1920,
			60,
			6000,
			160,
			2,
			true,
			true,
		},
		StreamTargetPreset{
			"youtube-shorts-1080p60",
			"YouTube Shorts Vertical (1080x1920 @ 60fps)",
			"YouTube Shorts",
			CanvasOrientation::Vertical,
			StreamProtocol::RTMPS,
			"rtmps://a.rtmps.youtube.com/live2",
			1080,
			1920,
			60,
			6000,
			160,
			2,
			true,
			false,
		},
		StreamTargetPreset{
			"instagram-reels-1080p30",
			"Instagram Reels Vertical (1080x1920 @ 30fps)",
			"Instagram Reels",
			CanvasOrientation::Vertical,
			StreamProtocol::RTMPS,
			"rtmps://live-upload.instagram.com:443/rtmp/",
			1080,
			1920,
			30,
			4500,
			128,
			2,
			false,
			true,
		},
		StreamTargetPreset{
			"kick-vertical-1080p60",
			"Kick Vertical (1080x1920 @ 60fps)",
			"Kick",
			CanvasOrientation::Vertical,
			StreamProtocol::RTMPS,
			"rtmps://fa723fc1b171.global-contribute.live-video.net/app/",
			1080,
			1920,
			60,
			6000,
			160,
			2,
			true,
			false,
		},
	};
}

std::vector<StreamTargetPreset> MultistreamPresets::VerticalTargets()
{
	std::vector<StreamTargetPreset> out;
	for (const auto &target : AllTargets()) {
		if (target.orientation == CanvasOrientation::Vertical) {
			out.push_back(target);
		}
	}
	return out;
}

MultistreamBundle MultistreamPresets::DefaultBundle()
{
	const auto all = AllTargets();
	MultistreamBundle bundle;
	bundle.id = "awesome-os-multistream-default";
	bundle.name = "Twitch Horizontal + TikTok / YouTube Shorts / Kick / Reels Vertical";
	bundle.description =
		"Out-of-the-box simultaneous 16:9 horizontal + 9:16 vertical multistreaming "
		"with Linked Scene Switching, Shared Encoder, and Backtrack Replay Buffer.";
	bundle.horizontal_primary = all.front();
	bundle.vertical_targets = VerticalTargets();
	bundle.linked_scene_switching = true;
	bundle.shared_encoder_with_main = true;
	bundle.streaming_match_main = true;
	bundle.recording_match_main = true;
	bundle.backtrack_enabled = true;
	bundle.backtrack_always_on = true;
	bundle.backtrack_duration_sec = 60;
	return bundle;
}

std::optional<StreamTargetPreset> MultistreamPresets::FindById(std::string_view id)
{
	for (const auto &target : AllTargets()) {
		if (target.id == id) {
			return target;
		}
	}
	return std::nullopt;
}

bool MultistreamPresets::ValidatePreset(const StreamTargetPreset &preset)
{
	if (preset.id.empty() || preset.display_name.empty() || preset.ingest_url.empty()) {
		return false;
	}
	if (preset.width == 0 || preset.height == 0 || (preset.width & 1u) != 0 || (preset.height & 1u) != 0) {
		return false;
	}
	if (preset.orientation == CanvasOrientation::Horizontal && preset.width <= preset.height) {
		return false;
	}
	if (preset.orientation == CanvasOrientation::Vertical && preset.height <= preset.width) {
		return false;
	}
	if (preset.fps < 24 || preset.fps > 120) {
		return false;
	}
	if (preset.video_bitrate_kbps < 500 || preset.video_bitrate_kbps > 50000) {
		return false;
	}
	if (preset.audio_bitrate_kbps < 64 || preset.audio_bitrate_kbps > 320) {
		return false;
	}
	return true;
}

std::string MultistreamPresets::SerializeVerticalConfigJson(const MultistreamBundle &bundle)
{
	const uint32_t width = bundle.vertical_targets.empty() ? 1080u : bundle.vertical_targets.front().width;
	const uint32_t height = bundle.vertical_targets.empty() ? 1920u : bundle.vertical_targets.front().height;
	const uint32_t bitrate =
		bundle.vertical_targets.empty() ? 6000u : bundle.vertical_targets.front().video_bitrate_kbps;
	const uint32_t audio =
		bundle.vertical_targets.empty() ? 160u : bundle.vertical_targets.front().audio_bitrate_kbps;

	std::ostringstream os;
	os << "{\n";
	os << "  \"preset_bundle\": \"" << EscapeJsonString(bundle.id) << "\",\n";
	os << "  \"canvas\": [\n";
	os << "    {\n";
	os << "      \"width\": " << width << ",\n";
	os << "      \"height\": " << height << ",\n";
	os << "      \"streaming_video_bitrate\": " << bitrate << ",\n";
	os << "      \"record_video_bitrate\": " << bitrate << ",\n";
	os << "      \"audio_bitrate\": " << audio << ",\n";
	os << "      \"streaming_match_main\": " << (bundle.streaming_match_main ? "true" : "false") << ",\n";
	os << "      \"recording_match_main\": " << (bundle.recording_match_main ? "true" : "false") << ",\n";
	os << "      \"stream_advanced_settings\": " << (bundle.shared_encoder_with_main ? "false" : "true") << ",\n";
	os << "      \"record_advanced_settings\": " << (bundle.shared_encoder_with_main ? "false" : "true") << ",\n";
	os << "      \"linked_scene_switching\": " << (bundle.linked_scene_switching ? "true" : "false") << ",\n";
	os << "      \"backtrack\": " << (bundle.backtrack_enabled ? "true" : "false") << ",\n";
	os << "      \"backtrack_always_on\": " << (bundle.backtrack_always_on ? "true" : "false") << ",\n";
	os << "      \"backtrack_seconds\": " << bundle.backtrack_duration_sec << ",\n";
	os << "      \"stream_outputs\": [\n";
	for (size_t i = 0; i < bundle.vertical_targets.size(); ++i) {
		const auto &t = bundle.vertical_targets[i];
		os << "        {\n";
		os << "          \"name\": \"" << EscapeJsonString(t.display_name) << "\",\n";
		os << "          \"stream_server\": \"" << EscapeJsonString(t.ingest_url) << "\",\n";
		os << "          \"stream_key\": \"\",\n";
		os << "          \"enabled\": " << (t.enabled_by_default ? "true" : "false") << "\n";
		os << "        }" << (i + 1 < bundle.vertical_targets.size() ? "," : "") << "\n";
	}
	os << "      ]\n";
	os << "    }\n";
	os << "  ]\n";
	os << "}\n";
	return os.str();
}

} // namespace awesome_obs
