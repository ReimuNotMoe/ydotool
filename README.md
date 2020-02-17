# ydotool
Generic Linux command-line automation tool (no X!)

[![pipeline status](https://gitlab.com/ReimuNotMoe/ydotool/badges/master/pipeline.svg)](https://gitlab.com/ReimuNotMoe/ydotool/pipelines)

#### Contents
- [Usage](#usage)
- [Examples](#examples)
- [Notes](#notes)
- [Packages](#packages)
- [Build](#build)

## Releases
- [Ubuntu 18.04](https://gitlab.com/ReimuNotMoe/ydotool/-/jobs/artifacts/master/browse/build?job=package:ubuntu:18.04)
- [Ubuntu 19.04](https://gitlab.com/ReimuNotMoe/ydotool/-/jobs/artifacts/master/browse/build?job=package:ubuntu:19.04)
- [Debian 9](https://gitlab.com/ReimuNotMoe/ydotool/-/jobs/artifacts/master/browse/build?job=package:debian:9)
- [Arch AUR](https://aur.archlinux.org/packages/ydotool-git/) (Thanks [@Depau](https://github.com/Depau))
- [openSUSE Tumbleweed / Leap 15.1](https://software.opensuse.org/package/ydotool) (Thanks [@cubesky](https://github.com/cubesky))
- [Static binary for Kernel 3.2+](https://gitlab.com/ReimuNotMoe/ydotool/-/jobs/artifacts/master/browse/build?job=package:static)

## Important Notes
Since Jun, 2019, I have little time to maintain this project because I'm striving to start an undertaking (instead of working [996](https://en.wikipedia.org/wiki/996_working_hour_system)). 

If you would like to have features you want implemented quickly, you could consider [donating](https://www.patreon.com/classicoldsong) to this project. This will allow me to allocate more time on this project.

Also, pull requests are always welcomed. Thanks in advance for your generous help.

## Usage
In most times, replace `x` with `y`. :P

Currently implemented command(s):
- `type` - Type a string
- `key` - Press keys
- `mousemove` - Move mouse pointer to absolute position
- `click` - Click on mouse buttons
- `recorder` - Record/replay input events


## Examples
Type some words:

    ydotool type 'Hey guys. This is Austin.'

Switch to tty1:

    ydotool key ctrl+alt+f1

Close a window in graphical environment:

    ydotool key Alt+F4

Move mouse pointer to 100,100:

    ydotool mousemove 100 100

Relatively move mouse pointer to -100,100:

    ydotool mousemove_relative -- -100 100

Mouse right click:

    ydotool click 2
    

## Notes
#### About the project
As of May, 2019, searching `wayland xdotool replacement` online won't get much useful results.

If you find this project useful, please consider to spread it.

#### Runtime
This program requires access to `/dev/uinput`. **This usually requires root permissions.**

You can use it on anything as long as it accepts keyboard/mouse/whatever input. For example, wayland, text console, etc.

#### Available key names
See `/usr/include/linux/input-event-codes.h`

#### About the --delay option
ydotool works differently from xdotool. xdotool sends X events directly to X server, while ydotool uses the uinput framework of Linux kernel to emulate an input device.

When ydotool runs and creates an virtual input device, it will take some time for your graphical environment (X11/Wayland) to recognize and enable the virtual input device. (Usually done by udev)

So, if the delay was too short, the virtual input device may not got recognized & enabled by your graphical environment in time.

In order to solve this problem, I made a persistent background service, ydotoold, to hold a persistent virtual device, and accept input from ydotool. When ydotoold is unavailable, ydotool will work without it.

#### New modular design
Now everyone can write their own tool to use with ydotool. Have a look at the `Tool` folder.

I will write some documents for this when I have time.

## Build
### Dependencies
- [uInputPlus](https://github.com/YukiWorkshop/libuInputPlus)
- [libevdevPlus](https://github.com/YukiWorkshop/libevdevPlus)
- boost::program_options

### Compile
Nearly all my projects use CMake. It's very simple:

    mkdir build
    cd build
    cmake ..
    make -j `nproc`

