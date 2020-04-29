ydotoold(8)

# NAME

ydotoold \- daemon for *ydotool*(1)

# SYNOPSIS

*ydotoold*

# DESCRIPTION

*ydotool* lets you programmatically (or manually) simulate
keyboard input and mouse activity, etc. It does this by writing
directly to */dev/uinput* so it generally needs to run as root.

When *ydotool*(1) runs and creates a virtual input device, it will take some time for your graphical environment (eg X11/Wayland) to recognize and enable the virtual input device. (Usually done by udev)

If the delay is too short, the virtual input device may not be recognized & enabled by the graphical environment in time.

In order to solve this problem, the *ydotoold* background service holds a persistent virtual device, and accepts input from *ydotool*(1). When *ydotoold*(1) is unavailable, *ydotool*(1) will work without it.

# AUTHOR

*ydotool*(1) and *ydotoold*(8) were written by ReimuNotMoe.

This man page by bob.hepple@gmail.com

# COPYRIGHT

MIT License

# SEE ALSO

*ydotool*(1)

Project site: <https://github.com/ReimuNotMoe/ydotool>
