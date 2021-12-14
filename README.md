# ydotool
Generic Linux command-line automation tool (no X!)

[![pipeline status](https://gitlab.com/ReimuNotMoe/ydotool/badges/master/pipeline.svg)](https://gitlab.com/ReimuNotMoe/ydotool/pipelines)

## Important Notes
### Current situation
It's simple: **Pay for it or fork it.**

Thanks for all the people that are supporting this project!

All backers and sponsors are listed [here](https://github.com/TheNeuronProject/BACKERS/blob/main/README.md)

Recently we've read [the story of faker.js and the poor developer behind it](https://github.com/Marak/faker.js/issues/1046). And independent software developers in China, like us, have 10 times more life pressure than the author of faker.js. Since ydotool has the opportunity to benefit large IT companies (these large IT companies are the main cause of life pressure here, such as the "996" working hours), we need to stop this from the beginning. ydotool only has 1% stars of faker.js has, so it's not too late ;)

This project will not receive any updates from us unless there are supports.

If you are uncomfortable with the new refactored version, you can simply `git checkout` an older version.

### How to support us
- Donate on [Patreon](https://www.patreon.com/classicoldsong)
- Buy our products on our [Tindie Store](https://www.tindie.com/stores/sudomaker/), if they are meaningful to you (please leave a message: `ydotool user`, so we can know that this purchase is for supporting ydotool)

### [READ THIS BEFORE COMPLAINING ANYTING ABOUT THE PRICING](https://christine.website/blog/open-source-broken-2021-12-11)

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

All dependencies will be configured by CPM automatically to save you from building & installing them manually. **So an Internet connection is required.**

The libraries, `libevdevPlus` and `libuInputPlus`, are too small and too troublesome to be complied as shared libraries. So now they are statically linked. As a result, the compiled binaries will only depend on `libc` and `libstdc++`.

CMake will no longer build the packages or install compiled files to system directories. Because it's complicated to build packages for every distro correctly using CPack, and every distribution has its own customs of filesystem hierarchy. I suggest you to copy compiled files to desired destination manually.


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
