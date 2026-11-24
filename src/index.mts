/**
 * @file src/index.mts
 * @description Public API for @awesome-os/obs: CAS hydration and launch helpers.
 */

export {
  applyKeepLast5Pin,
  ensureObsBinary,
  fetchGhostIndex,
  GHOST_CAS_BASE_URL,
  GHOST_INDEX_URL,
  getGhostCacheDir,
  getObsAssetKey,
  getObsExecutableRelPath,
  locateExtractedExecutable,
  resolveObsEntry,
  tryResolveObsEntry,
  type GhostCasEntry,
  type GhostCasHistoryItem,
  type GhostIndex,
  type HydratedObsResult,
} from './hydrate.mts';

export {
  buildLaunchPlan,
  getPackageVersion,
  parseCliArgs,
  runCli,
  type LaunchPlan,
  type ParsedCliArgs,
} from './cli.mts';
