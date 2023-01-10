YDOTOOL(1)
# NAME

ydotool - command-line _/dev/uinput_ automation tool

# SYNOPSIS

*ydotool* *cmd* _args_

*ydotool* *cmd* --help

# DESCRIPTION

*ydotool* lets you programmatically (or manually) simulate keyboard input and mouse activity, etc. It does this by writing directly to _/dev/uinput_ so it generally needs to run as root.

Currently implemented command(s):

*type*
	Type a string
*key*
	Press keys
*mousemove*
	Move mouse pointer to absolute position
*click*
	Click on mouse buttons

# KEYBOARD COMMANDS
*key* [*-d*,*--key-delay* _<ms>_] [_<KEYCODE:PRESSED>_ ...]

	Type a given keycode.
	
	e.g. 28:1 28:0 means pressing on the Enter button on a standard US keyboard.

	42:1 38:1 38:0 24:1 24:0 38:1 38:0 42:0 - "LOL"

	Non-interpretable values, such as 0, aaa, l0l, will only cause a delay.

	See `/usr/include/linux/input-event-codes.h' for available key codes (KEY_\*).

	Options:
	*-d*,*--key-delay* _<ms>_
		Delay time between keystrokes. Default 12ms.

*type* [*-D*,*--next-delay* _<ms>_] [*-d*,*--key-delay* _<ms>_] [*-f*,*--file* _<filepath>_] "_text_"

	Types text as if you had typed it on the keyboard.

	Options:

	*-d*,*--key-delay* _<ms>_
		Delay time between key events (up/down each). Default 12ms.

	*-D*,*--next-delay* _<ms>_
		Delay between strings. Default 0ms.

	*-f*,*--file* _<filepath>_
		Specify a file, the contents of which will be typed as if passed as an argument. The filepath may also be '-' to read from stdin.

	Example: to type 'Hello world!' you would do:
		ydotool type 'Hello world!'

# MOUSE COMMANDS

*mousemove* [*-a*,*--absolute*] _<x> <y>_
	Move the mouse to the relative X and Y coordinates on the screen.

	Options:
	*--absolute*
		Use absolute position

	Example: to move the cursor to absolute coordinates (100,100):
		ydotool mousemove --absolute 100 100

*click* [*-d*,*--next-delay* _<ms>_] [*-r*,*--repeat* _N_ ] [_button_ ...]
	Send a click.

	Options:
	*-d*,*--next-delay* _<ms>_
		Delay between input events (up/down, a compete click means doubled time). Default 25ms.

	*-r*,*--repeat* _N_
		Repeat entire sequence N times

	all mouse buttons are represented using hexadecimal numeric values, with an optional
	bit mask to specify if mouse up/down needs to be omitted.

	- 0x00 - LEFT
	- 0x01 - RIGHT
	- 0x02 - MIDDLE
	- 0x03 - SIDE
	- 0x04 - EXTR
	- 0x05 - FORWARD
	- 0x06 - BACK
	- 0x07 - TASK
	- 0x40 - Mouse down
	- 0x80 - Mouse up
    
	Examples:

	- 0x00: chooses left button, but does nothing (you can use this to implement extra sleeps)
	- 0xC0: left button click (down then up)
	- 0x41: right button down
	- 0x82: middle button up

	The '0x' prefix can be omitted if you want.

# AUTHOR

ydotool was written by ReimuNotMoe.

This man page by bob.hepple@gmail.com

# BUGS

When *ydotool*(1) runs and creates a virtual input device, it will take some time for your graphical environment (eg X11/Wayland) to recognize and enable the virtual input device. (Usually done by udev)

If the delay is too short, the virtual input device may not be recognized & enabled by the graphical environment in time.

In order to solve this problem, there is a persistent background service, *ydotoold*(1), to hold a persistent virtual device, and accept input from *ydotool*(1). When *ydotoold*(1) is unavailable, *ydotool*(1) will work without it.

# LICENCE
AGPLv3

# SEE ALSO

*ydotoold*(8)

Project site: <https://github.com/ReimuNotMoe/ydotool>
