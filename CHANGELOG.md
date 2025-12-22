# ChangeLog
This project is now refactored. (from v1.0.0)

Changes:
- Rewritten in pure C99
- No external dependencies
- Uses a lot less memory & no dynamic memory allocation

Breaking Changes:
- `recorder` removed because it's irrelevant. It will become a separate project
- Command chaining and `sleep` are removed because this should be your shell's job
- `ydotool` now must work with `ydotoold`
- Usage of `click` and `key` are changed

Good News:
- Some people can finally build this project offline
- `key` now (only) accepts keycodes, so it's not limited to a specific keyboard layout
- Now it's possible to implement support for different keyboard layouts in `type`
