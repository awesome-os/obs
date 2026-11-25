#include "dock-session-guardian.hpp"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;

#define CHECK(cond)                                                              \
  do {                                                                           \
    if (!(cond)) {                                                               \
      std::fprintf(stderr, "[FAIL] %s:%d: %s\n", __FILE__, __LINE__, #cond);     \
      std::exit(1);                                                              \
    }                                                                            \
  } while (0)

int main() {
  const fs::path tmp = fs::temp_directory_path() / "obs_dock_guardian_test";
  std::error_code ec;
  fs::remove_all(tmp, ec);
  const fs::path profile = tmp / "profile";
  const fs::path cookies = tmp / "cookies" / "twitch_frankl81";
  fs::create_directories(profile);
  fs::create_directories(cookies);

  const std::string valid_dock(96, 'A');
  {
    std::ofstream out(profile / "basic.ini", std::ios::binary);
    out << "[Auth]\nType=Twitch\n[Twitch]\nDockState=" << valid_dock << "\n";
  }

  // 1. Healthy session snapshots DockState
  CHECK(obs_dock_guardian_snapshot_if_healthy(profile.string().c_str(), false, true));
  CHECK(fs::exists(profile / "dock-state.last-good.json"));

  // 2. Safe Mode must NEVER overwrite the snapshot
  CHECK(!obs_dock_guardian_snapshot_if_healthy(profile.string().c_str(), true, false));

  // 3. Simulate crash: stale CEF LOCK + wiped [Twitch] DockState= on degraded shutdown
  {
    std::ofstream lock(cookies / "LOCK", std::ios::binary);
    lock << "stale-pid";
  }
  {
    std::ofstream out(profile / "basic.ini", std::ios::binary | std::ios::trunc);
    out << "[Auth]\nType=Twitch\n[Twitch]\nDockState=\n";
  }

  // Shutdown guard restores wiped DockState when exiting Safe Mode
  CHECK(obs_dock_guardian_guard_shutdown(profile.string().c_str(), true, false));
  {
    std::ifstream in(profile / "basic.ini", std::ios::binary);
    std::string ini((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    CHECK(awesome_obs::DockSessionGuardian::ExtractIniValue(ini, "Twitch", "DockState") == valid_dock);
  }

  // 4. Simulate hard crash wiping basic.ini + leaving LOCK -> PreBootRecover heals both
  {
    std::ofstream out(profile / "basic.ini", std::ios::binary | std::ios::trunc);
    out << "[General]\nName=twitch_frankl81\n";
  }
  auto res = awesome_obs::DockSessionGuardian::PreBootRecover(profile.string(), (tmp / "cookies").string());
  CHECK(res.cleared_cef_locks && res.locks_removed == 1);
  CHECK(res.restored_twitch_docks);
  CHECK(!fs::exists(cookies / "LOCK"));
  {
    std::ifstream in(profile / "basic.ini", std::ios::binary);
    std::string ini((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    CHECK(awesome_obs::DockSessionGuardian::ExtractIniValue(ini, "Twitch", "DockState") == valid_dock);
    CHECK(awesome_obs::DockSessionGuardian::ExtractIniValue(ini, "Auth", "Type") == "Twitch");
  }

  fs::remove_all(tmp, ec);
  std::printf("[PASS] OBS Crash-Proof Twitch Dock Session Guardian (4/4 crash recovery scenarios passed)\n");
  return 0;
}
