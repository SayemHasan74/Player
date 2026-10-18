# User constraints

- All future tests must be muted and run in the background while the user works.
- Never activate test windows, steal focus, or automatically minimize/restore windows on the user's desktop. Tests requiring visible interaction remain manual unless the user explicitly authorizes a dedicated session.
- Keep validation bounded. Use one short reproduction for a specific hypothesis; freeze changes before expensive acceptance runs. Do not enter repeated full benchmark/tuning loops.
- Part 1 is not accepted. Do not begin Part 2.
- Preserve every locked architecture and technology decision in master prompt.md.
- Locked visual references for the later UI part are saved in design/references/: player-controls.png (user Image 1, player UI) and welcome-page.png (user Image 2, welcome page). Read design/references/README.md and inspect both originals when implementing that part. Do not start UI implementation until authorized.
- Match those visual references as closely as possible, but do not use or copy the IINA logo. FlowPlayer will use a different, original logo; logo creation is deferred until requested.
- User revision on 2026-10-18: resident RAM below 500 MB is accepted. This overrides the original 150/300 MB budgets. Do not continue RAM profiling or ask for elevated heap tracing.
- User reported all five manual checks passed on 2026-10-18; evidence is in tests/manual-results.json. Preserve their reported status.
- User explicitly accepted a first-deployment-only startup exception on 2026-10-18. Normal fully closed launches must remain under 500 ms; retain measured initial timing in reports.
