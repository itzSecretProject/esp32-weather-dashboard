# Changelog

All notable changes to this project are documented in this file.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [1.0.0] - 2026-11-19

### Added

- Dashboard layout adapted from the "Weathry" concept: sidebar, current conditions,
  Today's Highlight, a 7-day forecast with a "Tomorrow" card, and a weather condition map.
- Live data from Open-Meteo (no API key), with the time zone detected automatically from the coordinates.
- Map markers for two extra, configurable cities.
- Display support for the ESP32-3248S035 and for an ESP32 DevKit with an ST7796 or ILI9488 SPI module.
- Flicker-free strip rendering with anti-aliased fonts and dithered gradients, so no PSRAM is needed.
- Desktop preview tool that renders the UI to PNG from JSON fixtures.
- CI workflow that builds every firmware target and renders the preview images.
