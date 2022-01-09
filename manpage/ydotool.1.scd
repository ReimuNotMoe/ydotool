YDOTOOL(1)

# NAME

ydotool - command-line _/dev/uinput_ automation tool

# SYNOPSIS

*ydotool* [OPTION...] *cmd* _args_ [ , *cmd* _args_ ... ]

*ydotool* [OPTION... ] *cmd* --help

# DESCRIPTION

*ydotool* lets you programmatically (or manually) simulate keyboard input and mouse activity, etc. It does this by writing directly to _/dev/uinput_ so it generally needs to run as root.

It's possible to chain multiple commands together, separated by a comma between two spaces.

Currently implemented command(s):

*type*
	Type a string
*key*
	Press keys
*mousemove*
	Move mouse pointer to absolute position
*click*
	Click on mouse buttons
*recorder*
	Record/replay input events
*sleep*
	sleep for a while

# OPTIONS

	*--socket-path arg*  _<path>_
		Socket path (default: /tmp/.ydotool_socket)

# KEYBOARD COMMANDS
*key* [*--up*] [*--down*] [*--next-delay* _<ms>_] [*--key-delay* _<ms>_] [*--repeat* _<times>_] [*--repeat-delay <ms>*] _<key sequence>_

	Type a given keystroke. Examples being "alt+r", "ctrl+J",
	"ctrl+alt+n", "backspace".

	Options:

	*--up*
		Only keyup
		
	*--down*
		Only keydown

	*--delay* _<ms>_
		Delay before starting to output keystrokes. Default 100ms.

	*--key-delay* _<ms>_
		Delay time between keystrokes. Default 12ms.

	*--repeat* _<times>_
		Times to repeat the key sequence.

	*--repeat-delay* _<ms>_
		Delay time between repetitions. Default 0ms.

	Generally, any valid name from _/usr/include/linux/input-event-codes.h_ will work. Multiple keys are separated by '+'.

	Each key sequence can be any number of modifiers and keys, separated by plus (+)

	For example: alt+r Alt+F4 CTRL+alt+f3 aLT+1+2+3 ctrl+Backspace 

	Since we are emulating keyboard input, combinations like Shift+# is invalid because typing a `#' involves pressing Shift and 3.

	Example: Switch to tty1:
		ydotool key ctrl+alt+f1

	Example: Close a window in graphical environment:
		ydotool key Alt+F4

*type* [*--next-delay* _<ms>_] [*--key-delay* _<ms>_] [*--texts* _arg_] [*--file* _<filepath>_] "_texts_"

	Types text as if you had typed it on the keyboard.

	Options:

	*--next-delay* _<ms>_
		Delay before starting typing. Default 100ms.

	*--key-delay* _<ms>_
		Delay time between keystrokes. Default 12ms.

	*--texts* _arg_
		Texts to type

	*--file* _<filepath>_
		Specify a file, the contents of which will be typed as if passed as an argument. The filepath may also be '-' to read from stdin.

	Example: to type 'Hello world!' you would do:
		ydotool type 'Hello world!'

# MOUSE COMMANDS

*mousemove* _<x> <y>_
	Move the mouse to the specific relative X and Y coordinates on the screen.

	Options:
	*--absolute*
		Use absolute position

	Example: to move the cursor to absolute coordinates (100,100):
		ydotool mousemove --absolute 100 100

*click*  [*--next-delay* _<ms>_] _button_
	Send a click. Buttons are: _left_, _right_ or _middle_

	Options:

	*--next-delay* _<ms>_
		Delay before click. Default 100ms.

	*--up*
		Only mouseup

	*--down*
		Only mousedown

	*--buttons* _<arg>_
		Buttons to press (left, right, middle)

	Example: Mouse middle click:
		ydotool click middle

*recorder* [*--record* _<devices>_] [*--replay* _<input files>_] [*--display*] [*--duration* _<ms>_] [*--devices* _<path>_] [*--file* _<path>_]

	Options:

	*--record* _<devices>_
		Devices to record from. Default is all, including non-keyboard devices.

	*--replay* _<input files>_
		The record file can't be replayed on an architecture with different endianness.

	*--display*
		Display

	*--duration* _<ms>_
		Record duration. Otherwise use SIGINT to stop recording.

	*--devices* _<path>_
		Devices, separated by comma, to record from. Default is all devices (default: "")

	*--file* _<path>_
		File to record to / replay from

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
