# ydotool
Generic Linux command-line automation tool (no X!)

## Usage
Replace `x` with `y`. :P

Currently implemented command(s):
- `type` - Type a string
- `key` - Press keys
- `mousemove` - Move mouse pointer to absolute position
- `mousemove_relative` - Move mouse pointer to relative position
- `click` - Click on mouse buttons

## Compatibility
This program requires access to `/dev/uinput`.

You can use it on anything as long as it accepts keyboard/mouse/whatever input. For example, wayland, text console, etc.

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

## Build
### Dependencies
- [uInputPlus](https://github.com/YukiWorkshop/libuInputPlus)
- boost::program_options

### Compile
Nearly all my projects use CMake. It's very simple:

    mkdir build
    cd build
    cmake ..
    make -j `nproc`
