# ydotool [![Build Status](https://github.com/ReimuNotMoe/ydotool/workflows/Build/badge.svg)](https://github.com/ReimuNotMoe/ydotool/actions/workflows/push_pr_build_cmake.yml) [![Release Status](https://github.com/ReimuNotMoe/ydotool/workflows/Release/badge.svg)](https://github.com/ReimuNotMoe/ydotool/actions/workflows/release_cmake.yml)

Generic Linux command-line automation tool (no X!)

**`ydotool` is not limited to Wayland.** You can use it on anything as long as it accepts keyboard/mouse/whatever input. For example, X11, text console, "RetroArch OS", fbdev apps (fbterm/mplayer/SDL1/LittleVGL/Qt Embedded), etc.

## Important Note
The man page is not always up to date. Please use `--help` to ensure correctness.

## ChangeLog
This project is now refactored. (v1.0.0)

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

## Usage
Currently implemented command(s):
- `type` - Type a string
- `key` - Press keys
- `mousemove` - Move mouse pointer to absolute position
- `click` - Click on mouse buttons

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

## Notes
#### Runtime
This program requires access to `/dev/uinput`. **This usually requires root permissions.**

#### Available key names
See `/usr/include/linux/input-event-codes.h`

#### Why a background service is needed
ydotool works differently from xdotool. xdotool sends X events directly to X server, while ydotool uses the uinput framework of Linux kernel to emulate an input device.

When ydotool runs and creates a virtual input device, it will take some time for your graphical environment (X11/Wayland) to recognize and enable the virtual input device. (Usually done by udev)

So, if the delay was too short, the virtual input device may not get recognized & enabled by your graphical environment in time.

In order to solve this problem, a persistent background service, ydotoold, is made to hold a persistent virtual device, and accept input from ydotool.

Since v1.0.0, the use of ydotoold is mandatory.

## Build
**CMake 3.4+ is required.**


### Compile

    mkdir build
    cd build
    cmake ..
    make -j `nproc`


## Troubleshooting
### Custom keyboard layouts
Currently, ydotool does not recognize if the user is using a custom keyboard layout. In order to comfortably use ydotool alongside a custom keyboard layout, the user could use one of the following fixes/workarounds:

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

#### Use a hardware-configurable keyboard
[As mentioned here](https://github.com/ReimuNotMoe/ydotool/issues/43#issuecomment-605921288), consider using a hardware-based configuration that supports using a custom layout without configuring it in software.

## Older Notes
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
