# SAYAHA_Offline-C-game

A small offline action-narrative game built around a shadowy city theme. The project is centered on a C/Raylib implementation and includes additional HTML/JavaScript assets and helper scripts for presentation and visual generation.

## Overview

This repository contains a playable prototype in which the player moves through a stylized city, enters shadow zones, talks to NPCs, and triggers a sequence of story-driven interactions. The game mixes exploration, dialogue, and simple platforming elements in a compact arcade presentation.

The project is intentionally lightweight and self-contained, making it easy to run locally without any external network dependency.

## Features

- Retro pixel-art city scene and atmospheric background
- Shadow takeover / interaction mechanic triggered inside dark zones
- NPC dialogue and branching-style conversations
- Small object interactions such as picking up and throwing a stick
- Water/reflection area, lighting effects, and a stylized title/end sequence
- Offline gameplay with a local executable build option

## Repository structure

- `main.c` — main game logic, rendering, dialogue system, world state, and gameplay loop
- `game.html` and `index.html` — browser/HTML presentation layers for the game experience
- `game.js` — JavaScript logic used by the browser-based version
- `bannerer.py` — helper script for asset/banner generation
- `graff.py` — utility script related to visual generation or export
- `icon.png` and `Cover.png` — branding and cover art
- `shadow_takeover.exe` — Windows executable build of the game

## Controls

- Enter: Start the game / return to title
- Left / A: Move left
- Right / D: Move right
- Up / W: Jump
- Down / S: Interact, pick up items, and trigger shadow actions depending on context
- Z / C: Advance dialogue in story scenes

## Running the game

### Option 1: Run the built Windows executable

Double-click or launch:

- `shadow_takeover.exe`

### Option 2: Build from source

The main game is implemented in C and uses Raylib. Compile `main.c` in a local Raylib-enabled C environment to produce a playable binary.

## Notes

- This project is a creative prototype with a strong visual identity and a narrative-driven structure.
- The game is designed for offline local play and is self-contained in the repository.
- Artwork and gameplay are intentionally stylized and compact rather than production-scale.

## Summary

SAYAHA_Offline-C-game is a compact shadow-themed offline game project that blends narrative scenes, exploration, and retro visuals into a single self-contained experience. It is best described as a locally playable C/Raylib game with supporting HTML and helper assets for presentation and tooling.
