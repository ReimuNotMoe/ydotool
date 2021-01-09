# ydotool
Generic Linux command-line automation tool (no X!)

[![pipeline status](https://gitlab.com/ReimuNotMoe/ydotool/badges/master/pipeline.svg)](https://gitlab.com/ReimuNotMoe/ydotool/pipelines)

## Packages & Releases
You may select an older tag to view the previous version.

- [Static build for Ubuntu 20.04](https://gitlab.com/ReimuNotMoe/ydotool/-/jobs/artifacts/master/browse/build?job=build:ubuntu:20.04)

## Important Notes
The project is now refactored, some redundant stuff are removed, and it no longer depends on boost. Since it's in a hurry, some new bugs may be introduced. You're welcome to find them out.
 
However my life is still very busy. I may still not have much time to maintain this project.

**The license has been changed to AGPLv3, to stop large tech companies from modifying this project and only use internally.** 

**In order to (hopefully) stop these free software license violations in China, the project is going to apply for a software copyright (软件著作权) in China. This will give me a way to sue them. If you want to contribute to this project from now on, you need to agree that your work will be copyrighted by me (only in China).**

As always, if you would like to have features you want implemented quickly, you could consider [donating](https://www.patreon.com/classicoldsong) to this project. This will allow me to allocate more time on this project.

## Usage
Currently implemented command(s):
- `type` - Type a string
- `sleep` - Sleep some time
- `key` - Press keys
- `mousemove` - Move mouse pointer to absolute position
- `click` - Click on mouse buttons
- `recorder` - Record/replay input events

Now it's possible to chain multiple commands together, separated by a comma between two spaces.

## Examples
Switch to tty1, wait 2 seconds, and type some words:

    ydotool key ctrl+alt+f1 , sleep 2000 , type 'Hey guys. This is Austin.'

Close a window in graphical environment:

    ydotool key Alt+F4

Relatively move mouse pointer to -100,100:

    ydotool mousemove -100 100

Move mouse pointer to 100,100:

    ydotool mousemove --absolute 100 100

Mouse right click:

    ydotool click right
    
## Notes
#### Runtime
This program requires access to `/dev/uinput`. **This usually requires root permissions.**

You can use it on anything as long as it accepts keyboard/mouse/whatever input. For example, wayland, text console, etc.

#### Available key names
See `/usr/include/linux/input-event-codes.h`

#### Why a background service is needed
ydotool works differently from xdotool. xdotool sends X events directly to X server, while ydotool uses the uinput framework of Linux kernel to emulate an input device.

When ydotool runs and creates a virtual input device, it will take some time for your graphical environment (X11/Wayland) to recognize and enable the virtual input device. (Usually done by udev)

So, if the delay was too short, the virtual input device may not got recognized & enabled by your graphical environment in time.

In order to solve this problem, I made a persistent background service, ydotoold, to hold a persistent virtual device, and accept input from ydotool. When ydotoold is unavailable, ydotool will try to work without it.

## Build
**CMake 3.14+ is required.**

Now all dependencies will be configured by CPM automatically to save you from building & installing them manually. So an Internet connection is required.

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
