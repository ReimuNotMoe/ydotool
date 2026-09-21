# ydotool [![Build Status](https://github.com/ReimuNotMoe/ydotool/workflows/Build/badge.svg)](https://github.com/ReimuNotMoe/ydotool/actions/workflows/push_pr_build_cmake.yml) [![Release Status](https://github.com/ReimuNotMoe/ydotool/workflows/Release/badge.svg)](https://github.com/ReimuNotMoe/ydotool/actions/workflows/release_cmake.yml)

Generic Linux command-line automation tool (no X!)

**`ydotool` is not limited to Wayland.** You can use it on anything as long as it accepts keyboard/mouse/whatever input. For example, X11, text console, "RetroArch OS", fbdev apps (fbterm/mplayer/SDL1/LittleVGL/Qt Embedded), etc.

## 2024 Roadmap
Our **ultra-lightweight** JavaScript runtime, *Resonance*, will be released in Q2 2024 in LGPL license.

ydotool will then be rewritten in JavaScript afterwards, to enable more people to understand the code & contribute.

**You have NO reason to reject this. The RAM consumption (RSS) will NOT exceed 1MB.**

## Important Note
The man page is not always up to date. Please use `--help` to ensure correctness.

## ChangeLog
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
- `key` accepts key names (`enter`, `f5`, `ctrl`, ...) as well as keycodes
- `type` supports the `us`, `de`, `fr`, `dvorak` and `colemak` keyboard layouts

## Usage
Currently implemented command(s):
- `click` - Click on mouse buttons
- `mousemove` - Move mouse pointer to absolute position
- `type` - Type a string
- `key` - Press keys
- `debug` - Print the socket, number of parameters and parameter values
- `bakers` - Show the honorable bakers
- `stdin` - Sends the key presses as it was a keyboard (i.e from ssh) See [PR #229](https://github.com/ReimuNotMoe/ydotool/pull/229)
- `do` - Perform a natural-language input request (optional build, see below)
- `mcp` - Run a Model Context Protocol computer-use server (optional build, see below)

## Examples
Switch to tty1 (Ctrl+Alt+F1), wait 2 seconds, and type some words:

    ydotool key 29:1 56:1 59:1 59:0 56:0 29:0; sleep 2; ydotool type 'echo Hey guys. This is Austin.'

Close a window in graphical environment (Alt+F4):

    ydotool key 56:1 62:1 62:0 56:0

Relatively move mouse pointer to -100,100:

    ydotool mousemove -x -100 -y 100

Move mouse pointer to 100,100:

    ydotool mousemove --absolute -x 100 -y 100

Mouse right click:

    ydotool click 0xC1

Mouse repeating left click:

    ydotool click --repeat 5 --next-delay 25 0xC0

Repeat the keyboard presses from stdin:

    ydotool stdin

Press keys by name:

    ydotool key ctrl:1 c:1 c:0 ctrl:0

Type text with a non-US layout:

    ydotool type --layout=de 'Grüße'

Interpret a natural-language request (requires a build with `ENABLE_TYPESAFE=ON`):

    ydotool do "copy the selected text"

    ydotool do --dry-run "scroll down three notches"

## Natural language commands
`ydotool do` turns a plain-language request into a single input action. It uses
[TypeSafe](https://typesafe.ai)'s System One API for interpretation, and ydotool
validates the result in code before sending any events. The request text is sent
to TypeSafe; no other data leaves the machine.

This command is optional and off by default because it adds a libcurl dependency:

    cmake -B build -DENABLE_TYPESAFE=ON
    cmake --build build

Set your API key and run:

    export TYPESAFE_API_KEY=...
    ydotool do "press ctrl+alt+t"
    ydotool do "double-click with the left mouse button"
    ydotool do "type hello world"

Recognized actions are keyboard combinations, standard application shortcuts
(copy, paste, undo, save, ...), mouse clicks, wheel scrolling, pointer movement
and typing literal text. Requests that describe several sequential actions are
refused: ydotool performs one action per invocation, and chaining is the shell's
job. Use `--dry-run` to see the resolved action without sending events, and
`--min-confidence` to control how certain an interpretation must be before it is
performed.

The interpreter can be re-validated against the reference phrase table with:

    python3 contrib/do_validation.py

## MCP computer-use server
`ydotool mcp` exposes input control and, on GNOME, window/focus/screenshot
perception to AI agents through the Model Context Protocol over stdio. It works
with MCP hosts such as Claude Code, Codex, Cursor or opencode.

This command is optional and off by default because it requires `atspi-2` and
`gio-2.0`:

    cmake -B build -DENABLE_MCP=ON
    cmake --build build

Register it with a host, for example Claude Code:

    claude mcp add ydotool -- /path/to/ydotool mcp

or point any MCP client at a stdio server that runs `ydotool mcp`.

Tools:

- `computer_get_state` - active window and window list with geometry
- `computer_screenshot` - full-screen PNG; the desktop asks the user to allow it
- `computer_focus_window` - best-effort focus, verified through accessibility events
- `computer_type`, `computer_press`, `computer_key`, `computer_click`,
  `computer_scroll`, `computer_move` - input actions
- `computer_do` - optional natural-language request (built with `ENABLE_TYPESAFE`)

Actions go to the focused window. Pass `expect_window` on an action to make it
refuse when another window is focused. Safety options:

- `--read-only` exposes perception tools only
- `--dry-run` reports planned actions without sending events
- `--require-focus` refuses actions when the active window is unknown
- `--allow-dangerous` permits ctrl+alt+delete, VT switches and magic sysrq
- `--rate-limit=N` caps actions per second (default 20)

`contrib/mcp_test.py` runs a protocol and event-level smoke test against a build.

## Notes
#### Runtime
`ydotoold` (daemon) program requires access to `/dev/uinput`. **This usually requires root permissions.**

#### Available key names
See `/usr/include/linux/input-event-codes.h`

#### Why a background service is needed
ydotool works differently from xdotool. xdotool sends X events directly to X server, while ydotool uses the uinput framework of Linux kernel to emulate an input device.

When ydotool runs and creates a virtual input device, it will take some time for your graphical environment (X11/Wayland) to recognize and enable the virtual input device. (Usually done by udev)

So, if the delay was too short, the virtual input device may not get recognized & enabled by your graphical environment in time.

In order to solve this problem, a persistent background service, ydotoold, is made to hold a persistent virtual device, and accept input from ydotool.

Since v1.0.0, the use of ydotoold is mandatory.

## Build
**CMake 3.22+ is required.**

### Build options
There are a few extra options that can be configured when running CMake

- BUILD_DOCS=ON|OFF - whether to build the documentation, depends on ``scdoc``. Default: ON
- SYSTEMD_USER_SERVICE=ON|OFF - whether to use systemd user service file, depends on ``systemd``. Default: ON
- SYSTEMD_SYSTEM_SERVICE=ON|OFF - whether to use systemd system service file, depends on ``systemd``. Default: OFF
- OPENRC=ON|OFF - whether to use openrc service file. Default: OFF (TBD)


### Compile

    mkdir build
    cd build
    cmake ..
    make -j `nproc`

If issues appears, check the build options, but try to install the dependecies:

Debian-based:

    sudo apt install scdoc

RHEL-based:

    sudo dnf install scdoc
## Troubleshooting
### Custom keyboard layouts
For the layouts built into `type` (`us`, `de`, `fr`, `dvorak`, `colemak`), select one with `ydotool type --layout=NAME`. Characters that require dead keys are skipped with a warning.

For any other layout, ydotool does not recognize the layout configured in the compositor. In order to comfortably use ydotool alongside a custom keyboard layout, the user could use one of the following fixes/workarounds:

#### Sway
In [sway](https://github.com/swaywm/sway), the process is [fairly easy](https://github.com/swaywm/sway/wiki#keyboard-layout). Following the instructions there, you would end up with something like:
```
input "16700:8197:DELL_DELL_USB_Keyboard" {
	xkb_layout "us,us"
	xkb_variant "dvorak,"
	xkb_options "grp:shifts_toggle, caps:swapescape"
}
```
The identifier for your keyboard can be obtained from the output of `swaymsg -t get_inputs`.

#### Hyprland

You can use [Per-device input configs](https://wiki.hyprland.org/Configuring/Keywords/#per-device-input-configs) in [Hyprland](https://hyprland.org/). Simply add following snippet to your config:

```
device:ydotoold-virtual-device {
    kb_layout = us
    kb_variant =
    kb_options =
}
```

#### Use a hardware-configurable keyboard
[As mentioned here](https://github.com/ReimuNotMoe/ydotool/issues/43#issuecomment-605921288), consider using a hardware-based configuration that supports using a custom layout without configuring it in software.

### Current situation
This project is now being maintained **thanks to all the people that are supporting this project!**

All backers and sponsors are listed [here](https://github.com/TheNeuronProject/BACKERS/blob/main/README.md).

### How to support us
- Donate on our [Patreon](https://www.patreon.com/classicoldsong)
- Buy our products on our [Official Store](https://su.mk/store), if they are meaningful to you (please leave a message: `ydotool user`, so we can know that this purchase is for supporting ydotool)

### More talks
[Article: "Open Source" is Broken](https://christine.website/blog/open-source-broken-2021-12-11)

Independent software developers in China, like us, have 10 times more life pressure than Marak, the author of faker.js. Since ydotool has the opportunity to benefit large IT companies who won't pay a penny to us, we've changed the license to AGPLv3. These large IT companies are the main cause of life pressure here, such as the "996" working hours.

**Marak's fate will repeat on all open source developers eventually (of course we aren't talking about those who were born in billionare families) if we just _keep fighting with each other and do nothing to improve the situation._ If you make open source software as well, don't hesitate to ask for donations if you actually _need_ them.**

Also make sure you understand all the terms of AGPLv3 before using this software.
