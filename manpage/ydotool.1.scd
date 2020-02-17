YDOTOOL(1)

# NAME

ydotool - command-line _/dev/uinput_ automation tool

# SYNOPSIS

*ydotool* *cmd* _args..._

*ydotool* *cmd* --help\fR

# DESCRIPTION

*ydotool* lets you programatically (or manually) simulate keyboard input and mouse activity, etc. It does this by writing directly to _/dev/uinput_ so it generally needs to run as root.

Currently implemented command(s):

*type*
	Type a string
*key*
	Press keys
*mousemove*
	Move mouse pointer to absolute position
*mousemove_relative*
	Move mouse pointer to relative position
*click*
	Click on mouse buttons
*recorder*
	Record/replay input events
*mouseup*
	Send a mouse up event.
*mousedown*
	Send a mouse down event.

# KEYBOARD COMMANDS
*key* [*--up*] [*--down*] [*--delay* _<ms>_] [*--key-delay* _<ms>_] [*--repeat* _<times>_] [*--repeat-delay <ms>*] [*--persist-delay <ms>*] _<key sequence>_

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

	*--persist-delay* _<ms>_
		Keep virtual device alive for _<ms>_ ms. Should be used in conjunction with *--down* or *--up*

	Generally, any valid name from _/usr/include/linux/input-event-codes.h_ will work. Multiple keys are separated by '+'.

	Each key sequence can be any number of modifiers and keys, separated by plus (+)
	For example: alt+r Alt+F4 CTRL+alt+f3 aLT+1+2+3 ctrl+Backspace 

	Since we are emulating keyboard input, combinations like Shift+# is invalid because typing a `#' involves pressing Shift and 3.

	Example: Switch to tty1:
		ydotool key ctrl+alt+f1

	Example: Close a window in graphical environment:
		ydotool key Alt+F4

*type* [*--delay* _<ms>_] [*--key-delay* _<ms>_] [*--args* _<N>_] [*--file* _<filepath>_] "_something to type_"

	Types text as if you had typed it on the keyboard.

	Options:

	*--delay* _<ms>_
		Delay before starting typing. Default 100ms.

	*--key-delay* _<ms>_
		Delay time between keystrokes. Default 12ms.

	*--args* _<N>_
		????

	*--file* _<filepath>_
		Specify a file, the contents of which will be be typed as if passed as an argument. The filepath may also be '-' to read from stdin.

	Example: to type 'Hello world!' you would do:
		ydotool type 'Hello world!'

# MOUSE COMMANDS

*mousemove* [*--delay* _<ms>_] _<x> <y>_
	Move the mouse to the specific X and Y coordinates on the screen.

	Options:
	*--delay* _<ms>_
		Delay before starting move. Default 100ms.

	Example: to move the cursor to absolute coordinates (100,100):
		ydotool mousemove 100 100

*mousemove_relative* [*--delay* _<ms>_] _<x>_ _<y>_
	Move the mouse x,y pixels relative to the current position of the mouse cursor.

	Options:
	*--delay* _<ms>_
		Delay before starting move. Default 100ms.

	Example: Relatively move mouse pointer to -100,100:
		ydotool mousemove_relative -- -100 100

*click*  [*--delay* _<ms>_] _button_
	Send a click. Buttons are: 1=left 2=right 3=middle

	Options:
	
	*--delay* _<ms>_
		Delay before click. Default 100ms.

	Example: Mouse right click:
		ydotool click 2

*recorder* [*--delay* _<ms>_] [*--record* _<devices>_] [*--replay* _<input files>_] [*--display*] [*--duration* _<ms>_]

	Options:

	*--delay* _<ms>_
		Delay time before start recording/replaying. Default 5000ms.

	*--record* _<devices>_
		Devices to record from. Default is all, including non-keyboard devices.

	*--replay* _<input files>_
		The record file can't be replayed on an architecture with different endianness.

	*--display*
		????

	*--duration* _<ms>_
		Record duration. Otherwise use SIGINT to stop recording.

*mouseup* [*--delay* _<ms>_] _button_
	Send a mouse up event.  Buttons are: 1=left 2=right 3=middle

	Options:

	*--delay* _<ms>_
		Delay before click. Default 100ms.

	Example: Mouse right click:
		ydotool click 2

*mousedown* [*--delay* _<ms>_] _button_
	Send a mouse down event.  Buttons are: 1=left 2=right 3=middle

	Options:

	*--delay* _<ms>_
		Delay before click. Default 100ms.

	Example: Mouse right click:
		ydotool click 2

# AUTHOR

ydotool was written by ReimuNotMoe.

This man page by bob.hepple@gmail.com

# BUGS

When *ydotool*(1) runs and creates a virtual input device, it will take some time for your graphical environment (eg X11/Wayland) to recognize and enable the virtual input device. (Usually done by udev)

If the delay is too short, the virtual input device may not be recognized & enabled by the graphical environment in time.

In order to solve this problem, there is a persistent background service, *ydotoold*(1), to hold a persistent virtual device, and accept input from *ydotool*(1). When *ydotoold*(1) is unavailable, *ydotool*(1) will work without it.

# COPYRIGHT
MIT License

# SEE ALSO

*ydotoold*(8)

Project site: <https://github.com/ReimuNotMoe/ydotool>
