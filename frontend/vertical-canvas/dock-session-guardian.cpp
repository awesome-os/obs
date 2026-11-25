#include "dock-session-guardian.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>

namespace fs = std::filesystem;

namespace awesome_obs {

namespace {

constexpr const char *kBasicIniName = "basic.ini";
constexpr const char *kSnapshotName = "dock-state.last-good.json";

std::string ReadFileText(const fs::path &p) {
  std::ifstream in(p, std::ios::binary);
  if (!in) return {};
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

bool WriteFileAtomic(const fs::path &p, const std::string &content) {
  std::error_code ec;
  fs::path tmp = p;
  tmp += ".tmp";
  {
    std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
    if (!out) return false;
    out.write(content.data(), static_cast<std::streamsize>(content.size()));
    out.flush();
    if (!out.good()) return false;
  }
  fs::rename(tmp, p, ec);
  if (ec) {
    // Windows rename over existing target fallback
    fs::remove(p, ec);
    ec.clear();
    fs::rename(tmp, p, ec);
  }
  return !ec;
}

std::string JsonEscape(const std::string &s) {
  std::string out;
  out.reserve(s.size() + 8);
  for (char c : s) {
    if (c == '\\') out += "\\\\";
    else if (c == '"') out += "\\\"";
    else if (c == '\n') out += "\\n";
    else if (c == '\r') out += "\\r";
    else out.push_back(c);
  }
  return out;
}

std::string ExtractJsonField(const std::string &json, const std::string &field) {
  const std::string needle = "\"" + field + "\"";
  std::size_t pos = json.find(needle);
  if (pos == std::string::npos) return {};
  pos = json.find(':', pos + needle.size());
  if (pos == std::string::npos) return {};
  pos = json.find('"', pos + 1);
  if (pos == std::string::npos) return {};
  ++pos;
  std::string out;
  while (pos < json.size() && json[pos] != '"') {
    if (json[pos] == '\\' && pos + 1 < json.size()) {
      ++pos;
    }
    out.push_back(json[pos++]);
  }
  return out;
}

std::string TrimCR(std::string line) {
  while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) {
    line.pop_back();
  }
  return line;
}

} // namespace

std::string DockSessionGuardian::ExtractIniValue(const std::string &ini_text, const std::string &section, const std::string &key) {
  std::istringstream ss(ini_text);
  std::string line;
  std::string current_section;
  const std::string prefix = key + "=";

  while (std::getline(ss, line)) {
    std::string clean = TrimCR(line);
    if (clean.size() >= 2 && clean.front() == '[' && clean.back() == ']') {
      current_section = clean.substr(1, clean.size() - 2);
      continue;
    }
    if (current_section == section && clean.rfind(prefix, 0) == 0) {
      return clean.substr(prefix.size());
    }
  }
  return {};
}

std::string DockSessionGuardian::UpsertIniValue(const std::string &ini_text, const std::string &section, const std::string &key, const std::string &value) {
  std::istringstream ss(ini_text);
  std::string line;
  std::vector<std::string> lines;
  while (std::getline(ss, line)) {
    lines.push_back(TrimCR(line));
  }

  const std::string header = "[" + section + "]";
  const std::string prefix = key + "=";
  std::string current_section;
  bool found_section = false;
  bool replaced = false;
  std::size_t section_end_idx = lines.size();

  for (std::size_t i = 0; i < lines.size(); ++i) {
    const std::string &l = lines[i];
    if (l.size() >= 2 && l.front() == '[' && l.back() == ']') {
      if (current_section == section && !replaced) {
        section_end_idx = i;
      }
      current_section = l.substr(1, l.size() - 2);
      if (current_section == section) found_section = true;
      continue;
    }
    if (current_section == section && l.rfind(prefix, 0) == 0) {
      lines[i] = prefix + value;
      replaced = true;
      break;
    }
  }

  if (!replaced) {
    if (found_section) {
      lines.insert(lines.begin() + static_cast<std::ptrdiff_t>(section_end_idx), prefix + value);
    } else {
      if (!lines.empty() && !lines.back().empty()) lines.push_back("");
      lines.push_back(header);
      lines.push_back(prefix + value);
    }
  }

  std::ostringstream out;
  for (std::size_t i = 0; i < lines.size(); ++i) {
    out << lines[i] << "\n";
  }
  return out.str();
}

bool DockSessionGuardian::ReadSnapshot(const std::string &profile_dir, DockGuardianSnapshot &out) {
  const fs::path snap_path = fs::path(profile_dir) / kSnapshotName;
  const std::string text = ReadFileText(snap_path);
  if (text.empty()) return false;
  out.twitch_dock_state = ExtractJsonField(text, "twitch_dock_state");
  out.general_dock_state = ExtractJsonField(text, "general_dock_state");
  out.auth_type = ExtractJsonField(text, "auth_type");
  return out.twitch_dock_state.size() >= kMinValidDockStateLen || out.general_dock_state.size() >= kMinValidDockStateLen;
}

bool DockSessionGuardian::WriteSnapshotAtomic(const std::string &profile_dir, const DockGuardianSnapshot &snap) {
  const fs::path snap_path = fs::path(profile_dir) / kSnapshotName;
  std::ostringstream json;
  json << "{\n"
       << "  \"schema\": \"awesome-os.obs.dock-guardian.v1\",\n"
       << "  \"auth_type\": \"" << JsonEscape(snap.auth_type) << "\",\n"
       << "  \"twitch_dock_state\": \"" << JsonEscape(snap.twitch_dock_state) << "\",\n"
       << "  \"general_dock_state\": \"" << JsonEscape(snap.general_dock_state) << "\"\n"
       << "}\n";
  return WriteFileAtomic(snap_path, json.str());
}

DockGuardianResult DockSessionGuardian::PreBootRecover(const std::string &profile_dir, const std::string &cookies_dir) {
  DockGuardianResult res;

  // 1. Clear stale CEF LOCK / SingletonLock files left behind by hard crashes
  if (!cookies_dir.empty()) {
    std::error_code ec;
    if (fs::exists(cookies_dir, ec)) {
      for (const auto &entry : fs::recursive_directory_iterator(cookies_dir, ec)) {
        if (!entry.is_regular_file(ec)) continue;
        const std::string fname = entry.path().filename().string();
        if (fname == "LOCK" || fname == "SingletonLock" || fname == "SingletonCookie") {
          std::error_code rm_ec;
          if (fs::remove(entry.path(), rm_ec)) {
            res.cleared_cef_locks = true;
            res.locks_removed++;
          }
        }
      }
    }
  }

  // 2. Restore wiped/truncated [Twitch] DockState= from last-known-good snapshot
  if (!profile_dir.empty()) {
    const fs::path ini_path = fs::path(profile_dir) / kBasicIniName;
    const std::string ini = ReadFileText(ini_path);
    const std::string current_twitch = ExtractIniValue(ini, "Twitch", "DockState");
    DockGuardianSnapshot snap;
    if (current_twitch.size() < kMinValidDockStateLen && ReadSnapshot(profile_dir, snap) &&
        snap.twitch_dock_state.size() >= kMinValidDockStateLen) {
      std::string healed = UpsertIniValue(ini, "Twitch", "DockState", snap.twitch_dock_state);
      if (!snap.auth_type.empty() && ExtractIniValue(healed, "Auth", "Type").empty()) {
        healed = UpsertIniValue(healed, "Auth", "Type", snap.auth_type);
      }
      if (WriteFileAtomic(ini_path, healed)) {
        res.restored_twitch_docks = true;
      }
    }
  }

  return res;
}

bool DockSessionGuardian::SnapshotIfHealthy(const std::string &profile_dir, bool safe_mode, bool twitch_docks_mounted) {
  if (safe_mode || !twitch_docks_mounted || profile_dir.empty()) {
    return false;
  }
  const fs::path ini_path = fs::path(profile_dir) / kBasicIniName;
  const std::string ini = ReadFileText(ini_path);
  if (ini.empty()) return false;

  DockGuardianSnapshot snap;
  snap.twitch_dock_state = ExtractIniValue(ini, "Twitch", "DockState");
  snap.general_dock_state = ExtractIniValue(ini, "General", "DockState");
  snap.auth_type = ExtractIniValue(ini, "Auth", "Type");

  if (snap.twitch_dock_state.size() < kMinValidDockStateLen) {
    return false;
  }
  return WriteSnapshotAtomic(profile_dir, snap);
}

bool DockSessionGuardian::GuardShutdown(const std::string &profile_dir, bool safe_mode, bool twitch_docks_mounted) {
  if (profile_dir.empty()) return false;
  if (!safe_mode && twitch_docks_mounted) {
    // Healthy shutdown — refresh snapshot
    return SnapshotIfHealthy(profile_dir, false, true);
  }
  // Degraded or Safe Mode shutdown — prevent empty [Twitch] DockState= overwrite
  const fs::path ini_path = fs::path(profile_dir) / kBasicIniName;
  const std::string ini = ReadFileText(ini_path);
  const std::string current_twitch = ExtractIniValue(ini, "Twitch", "DockState");
  DockGuardianSnapshot snap;
  if (current_twitch.size() < kMinValidDockStateLen && ReadSnapshot(profile_dir, snap) &&
      snap.twitch_dock_state.size() >= kMinValidDockStateLen) {
    std::string restored = UpsertIniValue(ini, "Twitch", "DockState", snap.twitch_dock_state);
    return WriteFileAtomic(ini_path, restored);
  }
  return false;
}

} // namespace awesome_obs

extern "C" bool obs_dock_guardian_pre_boot(const char *profile_dir, const char *cookies_dir) {
  auto res = awesome_obs::DockSessionGuardian::PreBootRecover(
      profile_dir ? profile_dir : "", cookies_dir ? cookies_dir : "");
  return res.cleared_cef_locks || res.restored_twitch_docks;
}

extern "C" bool obs_dock_guardian_snapshot_if_healthy(const char *profile_dir, bool safe_mode, bool twitch_docks_mounted) {
  return awesome_obs::DockSessionGuardian::SnapshotIfHealthy(
      profile_dir ? profile_dir : "", safe_mode, twitch_docks_mounted);
}

extern "C" bool obs_dock_guardian_guard_shutdown(const char *profile_dir, bool safe_mode, bool twitch_docks_mounted) {
  return awesome_obs::DockSessionGuardian::GuardShutdown(
      profile_dir ? profile_dir : "", safe_mode, twitch_docks_mounted);
}
