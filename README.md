# ydotool
Generic Linux command-line automation tool (no X!)

## Usage
Replace `x` with `y`. :P

Currently implemented command(s):
- type

## Compatibility
You can use it on anything as long as it accepts keyboard/mouse/whatever input. For example, wayland, text console, etc.

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
