# Current limitations — Part 3

The authoritative current measurements are in tests/PART3_RESULTS.md. The controls, seek bar, scene menus, tracks, toasts and worker-written settings are implemented; full acceptance is pending.

- Pause/resume still registers dropped frames: the final4K check recorded1 before pause and5 before enabling HQ, an increase of4. A short pre-unpause Windows timer-policy experiment did not eliminate this and was reverted. Stable visible cadence after resume is not certified.
- HQ's measured on/playing interval added no drops (5→5); switching off completed without crash/command error, but its separate transition counter and visual continuity were not certified. One drop had also appeared around the initial auto-hide transition. Seeks reset mpv's counter; finalzero values are not claimed as evidence of zero earlier drops.
- Exact seeks measured10ms absolute position error on both4K and the subtitle/multi-audio fixture. Hidden controls produced372renders/372scene swaps; paused produced0/0. Peak resident RAM across all raw samples was484.29MB, within the user-approved500MB limit.
- Real fullscreen/Mini interaction, visible tearing, black flashes, audible continuity, mouse-drag feel, styled ASS/karaoke appearance, physical multi-monitor and network-buffer visualization remain user tests under the explicit minimal-automation instruction. PGS/VobSub and FLAC fixtures were unavailable and are not marked tested.
- Normal-launch startup and long idle stability have not been revalidated as a new full suite. Earlier Part1/Part2 reports apply to their own executable hashes. The older Part1 VP9 two-drop result is not claimed fixed.
- Playlist buttons intentionally show placeholders; playlist behavior is Part5. Auto-resume/autoplay settings are saved flags only for Parts4/5. No Part4 work has started.

Part3 packaged executable SHA256:3073b3e79f5d11cecaa6a83ab05fb6bc095a6689b9136fb55befd2b89ebce2e5. All three final smoke processes used this hash. Old rejected decoder-off Mini reports remain archived; current Mini keeps video/audio/decoding running and hides only video.
