import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import { dirname, join, resolve } from 'node:path';
import test from 'node:test';
import { fileURLToPath } from 'node:url';
import {
  applyKeepLast5Pin,
  buildLaunchPlan,
  fetchGhostIndex,
  getObsAssetKey,
  getObsExecutableRelPath,
  parseCliArgs,
  tryResolveObsEntry,
  type GhostCasEntry,
} from '../src/index.mts';

const REPO_ROOT = resolve(dirname(fileURLToPath(import.meta.url)), '..');
const CLI_PATH = resolve(REPO_ROOT, 'src', 'cli.mts');

test('fetches live b8g-ghost cas-v1 index and checks obs-* entry gracefully', async () => {
  const index = await fetchGhostIndex(true);
  assert.ok(index.total_entries > 0, 'Expected non-empty cas-v1 index');

  const obsKey = getObsAssetKey();
  const obsEntry = await tryResolveObsEntry(obsKey, index);

  if (obsEntry) {
    assert.equal(obsEntry.filename, obsKey);
    assert.match(obsEntry.sha256, /^[a-f0-9]{64}$/);
    assert.ok(obsEntry.bytes > 10_000_000, 'Expected real OBS Studio archive (>10 MB)');
  } else {
    assert.equal(obsEntry, null);
  }
});

test('maps platform asset keys and executable paths accurately', () => {
  assert.equal(getObsAssetKey('win32', 'x64'), 'obs-win64.tar.zst');
  assert.equal(getObsAssetKey('linux', 'x64'), 'obs-linux-x64.tar.zst');
  assert.equal(getObsAssetKey('linux', 'arm64'), 'obs-linux-arm64.tar.zst');
  assert.equal(getObsAssetKey('darwin', 'arm64'), 'obs-macos-arm64.tar.zst');

  assert.equal(getObsExecutableRelPath('win32'), join('bin', '64bit', 'obs64.exe'));
  assert.equal(getObsExecutableRelPath('linux'), join('bin', 'obs'));
  assert.equal(getObsExecutableRelPath('darwin'), join('OBS.app', 'Contents', 'MacOS', 'OBS'));
});

test('applies Keep-Last-5 OBS_GHOST_SHA256 pin when present in history', () => {
  const latestSha = 'a'.repeat(64);
  const prevSha = 'b'.repeat(64);
  const entry: GhostCasEntry = {
    filename: 'obs-win64.tar.zst',
    sha256: latestSha,
    bytes: 165_000_000,
    version: '31.0.3',
    download_url: `https://github.com/awesome-os/b8g-ghost/releases/download/cas-v1/${latestSha}`,
    history: [
      {
        sha256: prevSha,
        bytes: 160_000_000,
        version: '31.0.2',
        retired_at: '2026-11-20T00:00:00Z',
      },
    ],
  };

  const pinned = applyKeepLast5Pin(entry, prevSha);
  assert.equal(pinned.sha256, prevSha);
  assert.equal(pinned.bytes, 160_000_000);
  assert.equal(pinned.version, '31.0.2');
  assert.ok(pinned.download_url.endsWith(`/${prevSha}`));

  const unpinned = applyKeepLast5Pin(entry, 'c'.repeat(64));
  assert.equal(unpinned.sha256, latestSha);
});

test('parses CLI flags (--dry-run, --version, --no-splash, passthrough)', () => {
  const parsed = parseCliArgs(['--dry-run', '--no-splash', '--portable', '--', '--version']);
  assert.equal(parsed.dryRun, true);
  assert.equal(parsed.showVersion, false);
  assert.deepEqual(parsed.forwardArgs, ['--portable', '--', '--version']);

  const ver = parseCliArgs(['-v']);
  assert.equal(ver.showVersion, true);
});

test('CLI --version and --dry-run execute cleanly and resolve CAS launch plan', async () => {
  const versionOut = execFileSync(process.execPath, [CLI_PATH, '--version'], {
    encoding: 'utf8',
  }).trim();
  assert.match(versionOut, /^@awesome-os\/obs 0\.1\.0 /);
  assert.ok(versionOut.includes(getObsAssetKey()));

  const dryRunOut = execFileSync(process.execPath, [CLI_PATH, '--dry-run', '--portable'], {
    encoding: 'utf8',
  });
  const plan = JSON.parse(dryRunOut) as Awaited<ReturnType<typeof buildLaunchPlan>>;
  assert.equal(plan.version, '0.1.0');
  assert.equal(plan.obs.assetKey, getObsAssetKey());
  assert.ok(plan.obs.status === 'resolved' || plan.obs.status === 'pending-ingest');
  if (plan.obs.executablePath) {
    assert.equal(plan.obs.workingDir, dirname(plan.obs.executablePath));
  }
  assert.deepEqual(plan.forwardArgs, ['--portable']);
});
