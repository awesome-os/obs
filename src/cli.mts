#!/usr/bin/env node
/**
 * @file src/cli.mts
 * @description Zero-installer `obs` CLI:
 *   - Hydrates debloated rolling-latest portable OBS Studio from
 *     `awesome-os/b8g-ghost @ cas-v1` into `~/.b8g/cas/obs/<sha256>/` if missing.
 *   - Spawns `bin/64bit/obs64.exe` (Win64) / `obs` detached with `cwd = bin/64bit`
 *     so OBS Studio locates all plugins, data, and CEF browser sources.
 *   - `--dry-run`: resolves CAS entries and prints the launch plan JSON without spawning.
 *   - `--version` / `-v`: prints version and platform CAS asset key.
 */

import { spawn } from 'node:child_process';
import { readFileSync } from 'node:fs';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import {
  ensureObsBinary,
  fetchGhostIndex,
  getGhostCacheDir,
  getObsAssetKey,
  getObsExecutableRelPath,
  tryResolveObsEntry,
} from './hydrate.mts';

export interface ParsedCliArgs {
  dryRun: boolean;
  showVersion: boolean;
  showHelp: boolean;
  forwardArgs: string[];
}

export interface LaunchPlan {
  version: string;
  cacheDir: string;
  obs: {
    assetKey: string;
    status: 'resolved' | 'pending-ingest';
    sha256: string | null;
    bytes: number | null;
    obsVersion: string | null;
    executablePath: string | null;
    workingDir: string | null;
  };
  forwardArgs: string[];
}

export function getPackageVersion(): string {
  try {
    const pkgPath = resolve(dirname(fileURLToPath(import.meta.url)), '..', 'package.json');
    const pkg = JSON.parse(readFileSync(pkgPath, 'utf8')) as { version?: string };
    return pkg.version ?? '0.1.0';
  } catch {
    return '0.1.0';
  }
}

export function parseCliArgs(argv: readonly string[]): ParsedCliArgs {
  const forwardArgs: string[] = [];
  let dryRun = false;
  let showVersion = false;
  let showHelp = false;
  let passthrough = false;

  for (const arg of argv) {
    if (!passthrough && arg === '--') {
      passthrough = true;
      forwardArgs.push(arg);
      continue;
    }
    if (!passthrough && arg === '--dry-run') {
      dryRun = true;
      continue;
    }
    if (!passthrough && (arg === '--version' || arg === '-v')) {
      showVersion = true;
      continue;
    }
    if (!passthrough && (arg === '--help' || arg === '-h')) {
      showHelp = true;
      continue;
    }
    if (!passthrough && arg === '--no-splash') {
      continue;
    }
    forwardArgs.push(arg);
  }

  return { dryRun, showVersion, showHelp, forwardArgs };
}

export async function buildLaunchPlan(parsed: ParsedCliArgs): Promise<LaunchPlan> {
  const idx = await fetchGhostIndex();
  const cacheDir = getGhostCacheDir();
  const obsKey = getObsAssetKey();
  const entry = await tryResolveObsEntry(obsKey, idx);
  const executablePath = entry
    ? join(cacheDir, 'obs', entry.sha256, getObsExecutableRelPath())
    : null;
  const workingDir = executablePath ? dirname(executablePath) : null;

  return {
    version: getPackageVersion(),
    cacheDir,
    obs: {
      assetKey: obsKey,
      status: entry ? 'resolved' : 'pending-ingest',
      sha256: entry?.sha256 ?? null,
      bytes: entry?.bytes ?? null,
      obsVersion: entry?.version ?? null,
      executablePath,
      workingDir,
    },
    forwardArgs: parsed.forwardArgs,
  };
}

export async function runCli(argv: readonly string[] = process.argv.slice(2)): Promise<void> {
  const parsed = parseCliArgs(argv);

  if (parsed.showVersion) {
    console.log(
      `@awesome-os/obs ${getPackageVersion()} (${process.platform}/${process.arch} | obs=${getObsAssetKey()})`,
    );
    return;
  }

  if (parsed.showHelp) {
    console.log(
      [
        `obs ${getPackageVersion()} — Debloated rolling-latest portable OBS Studio on b8g-ghost CAS`,
        '',
        'Usage: obs [options] [obs flags...]',
        '',
        'Options:',
        '  --dry-run     Resolve CAS entry and print launch plan JSON without launching',
        '  --no-splash   Skip Lumi Hydration Splash (native launcher)',
        '  -v, --version Print version and platform CAS asset key',
        '  -h, --help    Show this help message',
        '',
        'Environment:',
        '  OBS_GHOST_SHA256  Pin to a Keep-Last-5 SHA-256 from b8g-ghost cas-v1',
        '  OBS_PROGRESS=1    Emit GHOST_PROGRESS <pct> <msg> lines on stdout',
      ].join('\n'),
    );
    return;
  }

  if (parsed.dryRun) {
    const plan = await buildLaunchPlan(parsed);
    console.log(JSON.stringify(plan, null, 2));
    return;
  }

  const obs = await ensureObsBinary();

  if (process.env.OBS_VERBOSE === '1') {
    console.error(
      `[awesome-os/obs] executable=${obs.executablePath} cwd=${obs.workingDir} (cacheHit=${obs.cacheHit})`,
    );
  }

  const child = spawn(obs.executablePath, parsed.forwardArgs, {
    cwd: obs.workingDir,
    detached: true,
    stdio: 'ignore',
    windowsHide: false,
    env: {
      ...process.env,
      B8G_GHOST_CAS_DIR: getGhostCacheDir(),
    },
  });
  child.unref();
}

const isMain =
  process.argv[1] !== undefined &&
  resolve(process.argv[1]) === resolve(fileURLToPath(import.meta.url));

if (isMain) {
  await runCli();
}
