# Remaining Part 1 issues

## Confirmed acceptance failure

- VP9 4K30 with NVDEC dropped 2 frames in 319.633 seconds on the packaged, fully tested executable. The cause has not been established. Neither restoring default decoder concurrency nor the discarded render scheduling experiment resolved it. Full Part 1 acceptance remains incomplete.

## Verification limits

- The user passed the five manual functional checks on an earlier executable. Their hash is preserved in tests/manual-results.json; later performance changes have not been visually rechecked by automation.
- Current-build minimized CPU was not instrumented because automated desktop minimize/restore was prohibited while the user worked. Manual minimize/restore passed; pause measured 0.0130% mean CPU over 60.001 seconds, with zero video renders/callbacks.
- All automated playback tests were muted. Audible continuity during drag/resize has not been verified.
- The deployment's first video frame took 3337.955 ms from process launch. Once the engine was ready, local open-to-first-video-swap measured 88–127 ms. These are different measurements; only first-deployment window startup has an explicit user-approved exception.

## Passed checks and approved changes

- HEVC 4K60 Main 10 and H.264 1080p60: zero dropped frames over more than five minutes each, using NVDEC.
- Peak resident RAM 479.75 MB, below the user-approved 500 MB limit including cache.
- Normal fully closed launches: 414.714–474.325 ms. Initial deployment launch 1745.955 ms, explicitly accepted as a one-time exception.
- Twenty consecutive clean exits, with render context freed before mpv termination.

See tests/RESULTS.md and the retained JSON evidence for complete numbers. Part 2 is not implemented. Generated binaries, downloaded SDKs and media are excluded from Git; see README.md to build locally.
