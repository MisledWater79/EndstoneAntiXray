# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

### Added
- `max_ms_per_tick` config option (default `5`) limiting how long chunk scanning may run each tick

### Changed
- Chunk scanning is now queued and spread across ticks instead of running all at once, removing lag spikes when players join, teleport or fly
- Only the sent chunk's own 16x16 columns are scanned, the Nether scan starts at y=0, and End chunks are skipped
- Explosions re-reveal ores once instead of once per nearby player
- Fewer block lookups and string copies when checking and hiding ores; config values are cached

## [0.1.1] - 2026-06-28

### Fixes
- Fixed as bug where players in different dimensions from explosions would fail to pass event

## [0.1.0] - 2026-06-26

### Added
- Updates viewable ores around exploded blocks and entities
- Config file
