# Clapperboard icon integration

Build SHA256625823347456be2c97a065e9356ffa2bdf86120eea1ee8ecf287bdd51dfbe8a2. User's circular reel removed; transparent PNG and seven Windows ICO sizes retained. Applied to executable resource1, Qt window/taskbar icon and welcome screen. No player/render settings changed.

Build/deploy passed. Native PE resource lookup found the embedded group icon. One three-second requested isolated muted/background Home check loaded successfully, never activated, reported no QML/image-resource errors and exited cleanly. PNG transparent corner and ICO size packaging were verified by scripts/make-icon.ps1. Explorer/taskbar visual cache refresh remains a user observation; no native UI automation was performed.

Evidence: tests/results/icon-smoke/{summary.json,flowplayer.log}. Assets and built-in imagegen edit prompt: assets/README.md. Earlier UI/performance reports remain scoped to their original hashes; no broad performance run was repeated for this branding change.
