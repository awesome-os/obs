# Built-in Native Vertical Canvas & Multi-Stream (`frontend/vertical-canvas/`)

> **Status:** Permanent Built-in Native OBS Frontend Core Feature (Zero External Plugin DLL)  
> **Upstream Attribution:** Hard-forked from [`Aitum/obs-vertical-canvas`](https://github.com/Aitum/obs-vertical-canvas) (`v1.6.8`) — Copyright © Aitum & Exeldro, licensed under **GNU General Public License v2.0 (GPL-2.0)** (full license preserved at [`frontend/vertical-canvas/LICENSE`](../frontend/vertical-canvas/LICENSE)).

---

## 1. Architecture: Permanent Native Core (Zero External Plugin DLL)

Upstream `obs-vertical-canvas` ships as an external dynamically loaded OBS plugin (`MODULE` `.dll`/`.so`/`.dylib`) relying on `OBS_DECLARE_MODULE()`, `obs_module_load()`, `obs_module_post_load()`, `obs_module_unload()`, and an external `api.aitum.tv` HTTP update checker.

In **`awesome-os/obs`**, Vertical Canvas is permanently integrated as a statically linked OBS frontend library (`add_library(obs-vertical-canvas STATIC ...)`) under [`frontend/vertical-canvas/`](../frontend/vertical-canvas/):

- **Zero External Plugin DLL:** Compiled with `-DOBS_VERTICAL_CANVAS_BUILTIN=1` and linked directly into the `obs-studio` / `obs` frontend target via [`CMakeLists.txt`](../CMakeLists.txt) and [`frontend/vertical-canvas/CMakeLists.txt`](../frontend/vertical-canvas/CMakeLists.txt).
- **Native Lifecycle Entrypoints ([`vertical-canvas-core.hpp`](../frontend/vertical-canvas/vertical-canvas-core.hpp)):**
  - `extern "C" bool obs_vertical_canvas_init(void)` — registers `audio_wrapper_source`, `multi_canvas_source`, proc handlers, and frontend event callbacks.
  - `extern "C" void obs_vertical_canvas_post_init(void)` — initializes the built-in `VerticalCanvasDock` with Multi-Stream defaults and registers `obs-websocket` vendor requests.
  - `extern "C" void obs_vertical_canvas_shutdown(void)` — unregisters vendor requests and frontend event callbacks cleanly on teardown.
  - `awesome_obs::VerticalCanvasCore` & `awesome_obs::VerticalCanvasFrontendGuard` — RAII C++ wrapper for `OBSBasic` / `obs-app.cpp` startup and teardown:

```cpp
#include "vertical-canvas-core.hpp"

// Inside OBSBasic::OBSInit() / obs-app.cpp startup:
auto verticalCanvas = std::make_unique<awesome_obs::VerticalCanvasFrontendGuard>();
// After main window & docks are ready:
verticalCanvas->PostInitialize();
// Automatic obs_vertical_canvas_shutdown() on destruction during OBS teardown.
```

- **Zero External Update Telemetry:** Removed `file-updater.{c,h}` and external `https://api.aitum.tv/plugin/vertical` update pings; Vertical Canvas updates atomically with `@awesome-os/obs`.
- **Zero `obs_module_t` Dependency:** [`vertical-canvas-builtin-compat.hpp`](../frontend/vertical-canvas/vertical-canvas-builtin-compat.hpp) routes localization (`obs_vertical_canvas_text`), profile config paths (`obs_vertical_canvas_config_path`), and bundled data paths (`obs_vertical_canvas_data_file`) natively through `obs-frontend-api`.

---

## 2. Built-in 1-Click Multi-Stream Presets

[`frontend/vertical-canvas/multistream-presets.hpp`](../frontend/vertical-canvas/multistream-presets.hpp) and [`multistream-presets.cpp`](../frontend/vertical-canvas/multistream-presets.cpp) provide out-of-the-box simultaneous **Horizontal (16:9) + Vertical (9:16)** multistreaming presets wired directly into `CanvasDock` first-boot initialization:

| Preset ID | Platform | Orientation | Resolution & FPS | Video / Audio Bitrate | Protocol & Ingest |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `twitch-horizontal-1080p60` | **Twitch** | Horizontal (16:9) | `1920x1080 @ 60fps` | `6000 kbps` / `160 kbps` | RTMPS (`ingest.global-contribute.live-video.net`) |
| `tiktok-vertical-1080p60` | **TikTok Live** | Vertical (9:16) | `1080x1920 @ 60fps` | `6000 kbps` / `160 kbps` | RTMPS (`push-rtmp-f5-tt.tiktokcdn.com`) |
| `youtube-shorts-1080p60` | **YouTube Shorts** | Vertical (9:16) | `1080x1920 @ 60fps` | `6000 kbps` / `160 kbps` | RTMPS (`a.rtmps.youtube.com/live2`) |
| `instagram-reels-1080p30` | **Instagram Reels** | Vertical (9:16) | `1080x1920 @ 30fps` | `4500 kbps` / `128 kbps` | RTMPS (`live-upload.instagram.com:443/rtmp/`) |
| `kick-vertical-1080p60` | **Kick** | Vertical (9:16) | `1080x1920 @ 60fps` | `6000 kbps` / `160 kbps` | RTMPS (`fa723fc1b171.global-contribute.live-video.net`) |

### Zero-Config Creator Defaults (`MultistreamPresets::DefaultBundle()`)

- **Linked Scene Switching:** `linked_scene_switching = true` (switching horizontal main scenes automatically switches linked vertical scenes).
- **Shared Encoder with Main OBS:** `shared_encoder_with_main = true` (`stream_advanced_settings = false`, `streaming_match_main = true`, `recording_match_main = true`).
- **Backtrack Replay Buffer:** `backtrack_enabled = true`, `backtrack_always_on = true`, `backtrack_duration_sec = 60` for instant vertical highlight clips.

---

## 3. Native C++17 Verification (Zero WSL)

Compile and execute the standalone presets & core header smoke test natively with Zig C++:

```powershell
.\bin\zig.cmd c++ -std=c++17 -O2 -Wall -Wextra -Werror `
  frontend/vertical-canvas/multistream-presets.cpp `
  frontend/vertical-canvas/tests/vertical-canvas-presets-test.cpp `
  -I frontend/vertical-canvas -o vertical-canvas-presets-test.exe
.\vertical-canvas-presets-test.exe
```

---

## 4. License & Attribution

The built-in Vertical Canvas module in `frontend/vertical-canvas/` is a derivative work of [`Aitum/obs-vertical-canvas`](https://github.com/Aitum/obs-vertical-canvas) (Copyright © Aitum / Exeldro) and is distributed under the **GNU General Public License v2.0 (GPL-2.0)** in compliance with Section 2 of the GPL-2.0. See [`frontend/vertical-canvas/LICENSE`](../frontend/vertical-canvas/LICENSE).
