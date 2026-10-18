# Current limitations

Part 2 now uses the user-approved hide-only Mini policy in prompt2-amendments.md. The current report is tests/PART2_RESULTS.md.

- The short packaged HEVC 4K60 hide/reveal check passed with zero dropped frames, no A/V desync warning, continuous timeline, selected video track, clean exit and peak resident working set 477.64 MB. This is a short check, not a long playback benchmark.
- Actual native mode transitions, fullscreen control visibility/clicks, visible tearing, flashes, audible continuity, rapid desktop toggles, monitor/DPI changes and drag/resize remain user tests by explicit request. No current-build manual pass is claimed yet.
- Normal-launch startup and long idle metrics have not been remeasured for this new executable. The approved first-deployment startup exception remains in effect.
- The original Part 1 VP9 test had two drops in five minutes on an earlier executable; it has not been rerun on this build.
- Earlier decoder-off Mini restoration produced 67 drops, an A/V desync warning and a 512.09 MB transient working set. That rejected design has been removed, not hidden behind reset counters. Its report remains in tests/PART2_DECODER_RESTART_RESULTS.md and raw evidence remains in tests/results/part2-final-basic-20261018. Those are historical results, not measurements of the current executable.

Mini now continues decoding while its video is hidden. It therefore does not claim the old decoder-off resource savings. No Part 3 implementation has started.
