YDOTOOL(1)
# NAME

ydotool - command-line _/dev/uinput_ automation tool

# SYNOPSIS

*ydotool* *cmd* _args_

*ydotool* *cmd* --help

# DESCRIPTION

*ydotool* lets you programmatically (or manually) simulate keyboard input and mouse activity, etc.
The *ydotoold*(8) daemon must be running.


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
	(where :1 for pressed means the key is down and then :0 means the key is released)

	42:1 38:1 38:0 24:1 24:0 38:1 38:0 42:0 - "LOL"

	Non-interpretable values, such as 0, aaa, l0l, will only cause a delay.

	See `/usr/include/linux/input-event-codes.h' for available key codes (KEY_\*).

	You can find the key name/number your keyboard is sending to libinput by running `sudo libinput record` and then selecting your keyboard from the list it will show you the libinput proper key name and number for each key you press.

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

# YDOTOOL SOCKET

The socket to write to for *ydotoold*(8) can be changed by the environment variable YDOTOOL_SOCKET.

# AUTHOR

ydotool was written by ReimuNotMoe.

This manpage was written by bob.hepple@gmail.com but updated since.

# LICENCE
AGPLv3

# SEE ALSO

*ydotoold*(8)

Project site: <https://github.com/ReimuNotMoe/ydotool>
