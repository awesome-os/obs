# `@awesome-os/obs`

Debloated rolling-latest portable **OBS Studio** hydrated on demand from [`awesome-os/b8g-ghost`](https://github.com/awesome-os/b8g-ghost) (`cas-v1`) with a `~118 KB` native 4-OS C11 **Lumi Hydration Splash** launcher (Keep-Last-5, zero installer).

## Quick Install & Usage

### Windows (PowerShell 1-liner)

```powershell
irm https://github.com/awesome-os/obs/releases/latest/download/install.ps1 | iex
```

### Linux (x64 / ARM64) & macOS (Apple Silicon)

```sh
curl -fsSL https://github.com/awesome-os/obs/releases/latest/download/install.sh | sh
```

### CLI Usage

```powershell
# Inspect installed launcher version & help
obs --version
obs --help

# Launch OBS Studio (with optional flags forwarded)
obs
obs --startrecording

# Optional: store hydrated CAS layers on a custom drive/path
$env:B8G_GHOST_CACHE_DIR = "D:\b8g-cas"   # POSIX: export B8G_GHOST_CACHE_DIR=/data/b8g-cas
obs
```

### Releases & Verification

- **Latest Release:** [`awesome-os/obs` Releases](https://github.com/awesome-os/obs/releases/latest) — includes `SHA256SUMS.txt` and the 5-OS matrix (`win64`, `linux-x64`, `linux-arm64`, `macos-arm64`, `wasm32-wasi`) built via [`awesome-os/maintenance-workflows`](https://github.com/awesome-os/maintenance-workflows).

## Highlights

- **Zero Installer & Zero Bloat:** Hydrates directly into `~/.b8g/cas/obs/<sha256>/` and spawns `bin/64bit/obs64.exe` (Win64) / `obs` detached with `cwd = bin/64bit` so OBS Studio finds all plugins, data files, and CEF browser sources.
- **SHA-256 Verified Ghost CAS (`cas-v1`):** Downloads `obs-win64.tar.zst` (plus Linux/macOS targets), verifies SHA-256 before extraction, and supports Keep-Last-5 rollback via `OBS_GHOST_SHA256=<sha256>`.
- **Instant `< 15 ms` Lumi Hydration Splash:** Native 4-OS C11 launcher (`src/obs_launcher.c`) displays the embedded Lumi White Dragon icon on Windows and streams live `GHOST_PROGRESS <pct> <msg>` updates during cold CAS hydration.
- **Erasable TypeScript Direct Execution:** Zero build/transpile step (`erasableSyntaxOnly: true`, `noEmit: true`), runs directly on Node 22.18+ / Node 24.

## Usage

### Native Launcher (`obs.exe` / `obs-linux-x64` / `obs-linux-arm64` / `obs-macos-arm64`)

```powershell
# Launch OBS Studio with Lumi Hydration Splash
.\dist\obs.exe

# Inspect version & CAS asset key
.\dist\obs.exe --version
```

### Node CLI (`src/cli.mts`)

```powershell
# Print version and platform CAS key
node src/cli.mts --version

# Resolve CAS index and print launch plan JSON without spawning
node src/cli.mts --dry-run

# Stream GHOST_PROGRESS lines during hydration
$env:OBS_PROGRESS="1"; node src/cli.mts
```

## Environment Variables

| Variable | Description |
| :--- | :--- |
| `OBS_GHOST_SHA256` | Pin to a Keep-Last-5 historical SHA-256 from `b8g-ghost @ cas-v1`. |
| `OBS_PROGRESS=1` | Emit `GHOST_PROGRESS <pct> <msg>` lines on stdout during hydration. |
| `OBS_CLI_PATH` | Override path to `src/cli.mts` for the native C11 launcher. |
| `B8G_GHOST_CACHE_DIR` | Override default `~/.b8g/cas` cache directory. |

## Build & Test

```powershell
# Run CAS & CLI test suite
node --test test/obs-cas.test.mts

# Compile Windows native launcher (~118 KB with embedded Lumi icon)
zig cc -target x86_64-windows-gnu -std=c11 -Os -s -Wl,--subsystem,windows -Wall -Wextra src/obs_launcher.c lumi.res -lgdi32 -luser32 -o dist/obs.exe

# Compile Linux x64 / arm64 static musl & macOS arm64 launchers
zig cc -target x86_64-linux-musl -static -std=c11 -Os -s -Wall -Wextra src/obs_launcher.c -o dist/obs-linux-x64
zig cc -target aarch64-linux-musl -static -std=c11 -Os -s -Wall -Wextra src/obs_launcher.c -o dist/obs-linux-arm64
zig cc -target aarch64-macos -std=c11 -Os -s -Wall -Wextra src/obs_launcher.c -o dist/obs-macos-arm64
```

## License

Dual-licensed under **GPL-2.0-or-later OR MIT**.

## Quick Install & Run (Windows / Linux / macOS)

```sh
# Install / hydrate from v11 CAS via holo-pm
holo-pm install repository://github.com/awesome-os/obs@main

# Or project & run directly via holo-project (0-byte hardlink)
holo-project "project://node/obs?from=repository://github.com/awesome-os/obs@main"
```
