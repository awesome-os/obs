#pragma once

#include <cstddef>
#include <string>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Pre-boot crash recovery hook (call before CEF / OBSBasic dock init):
 *  1. Clears stale Chromium/CEF LOCK files in cookies_dir so Twitch browser docks do not hang/blank.
 *  2. If basic.ini is missing [Twitch] DockState= (or it was wiped), restores it atomically from
 *     dock-state.last-good.json.
 * Returns true if any recovery action was performed.
 */
bool obs_dock_guardian_pre_boot(const char *profile_dir, const char *cookies_dir);

/**
 * Healthy-session snapshot hook:
 *  - Refuses to snapshot if safe_mode == true or twitch_docks_mounted == false.
 *  - Atomically writes valid [Twitch] DockState= and [General] DockState= to dock-state.last-good.json.
 */
bool obs_dock_guardian_snapshot_if_healthy(const char *profile_dir, bool safe_mode, bool twitch_docks_mounted);

/**
 * Shutdown guard hook (call right after OBSBasic::SaveProject):
 *  - If shutting down from Safe Mode or a degraded session where Twitch docks failed to mount,
 *    re-injects the last-known-good [Twitch] DockState= so OBS never persists an empty dock layout.
 */
bool obs_dock_guardian_guard_shutdown(const char *profile_dir, bool safe_mode, bool twitch_docks_mounted);

#ifdef __cplusplus
} // extern "C"

namespace awesome_obs {

struct DockGuardianSnapshot {
  std::string twitch_dock_state;
  std::string general_dock_state;
  std::string auth_type;
};

struct DockGuardianResult {
  bool cleared_cef_locks = false;
  bool restored_twitch_docks = false;
  bool snapshot_saved = false;
  std::size_t locks_removed = 0;
};

class DockSessionGuardian {
public:
  static constexpr std::size_t kMinValidDockStateLen = 64;

  static DockGuardianResult PreBootRecover(const std::string &profile_dir, const std::string &cookies_dir);
  static bool SnapshotIfHealthy(const std::string &profile_dir, bool safe_mode, bool twitch_docks_mounted);
  static bool GuardShutdown(const std::string &profile_dir, bool safe_mode, bool twitch_docks_mounted);

  static bool ReadSnapshot(const std::string &profile_dir, DockGuardianSnapshot &out);
  static bool WriteSnapshotAtomic(const std::string &profile_dir, const DockGuardianSnapshot &snap);
  static std::string ExtractIniValue(const std::string &ini_text, const std::string &section, const std::string &key);
  static std::string UpsertIniValue(const std::string &ini_text, const std::string &section, const std::string &key, const std::string &value);
};

} // namespace awesome_obs
#endif
