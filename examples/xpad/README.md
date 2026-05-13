# GETXPAD examples

These examples use the Xbox-style `GETXPAD` function from gfxlib. `GETXPAD`
itself is a compiler intrinsic and does not require an include file.

- `first-pad.bas` shows the smallest useful polling loop for pad 0 without
  including any `.bi` helper.
- `active-state.bas` scans pad slots and shows which pads are active, missing, or previously disconnected.
- `stick-view.bas` gives a visual display of sticks, triggers, d-pad, and buttons.

All three examples exit with `Esc` on a keyboard, or `Start` + `Select` on a connected controller.
