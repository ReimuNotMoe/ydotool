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
*stdin*
	Resend all keypresses as a keyboard (i.e. from ssh)
*do*
	Interpret a natural-language input request (only in builds with ENABLE_TYPESAFE)
*mcp*
	Run a Model Context Protocol computer-use server (only in builds with ENABLE_MCP)

# KEYBOARD COMMANDS
*key* [*-d*,*--key-delay* _<ms>_] [_<KEY:PRESSED>_ ...]

	Type a given key.

	_KEY_ is an evdev keycode (decimal, or hexadecimal with a 0x prefix) or a key
	name such as *enter*, *esc*, *f5*, *ctrl* or *a*. _PRESSED_ is *1* for key down
	and *0* for key up; when omitted, the key is pressed.

	e.g. enter:1 enter:0 means pressing and releasing the Enter key.

	42:1 38:1 38:0 24:1 24:0 38:1 38:0 42:0 - "LOL"

	Unknown keys and invalid pressed values are an error.

	See `/usr/include/linux/input-event-codes.h' for available key codes (KEY_\*).

	You can find the key name/number your keyboard is sending to libinput by running `sudo libinput record` and then selecting your keyboard from the list it will show you the libinput proper key name and number for each key you press.

	Options:
	*-d*,*--key-delay* _<ms>_
		Delay time between keystrokes. Default 12ms.

*type* [*-D*,*--next-delay* _<ms>_] [*-d*,*--key-delay* _<ms>_] [*-l*,*--layout* _<name>_] [*-f*,*--file* _<filepath>_] "_text_"

	Types text as if you had typed it on the keyboard.

	Input is decoded as UTF-8. Escape sequences (\n, \t, \\, \xHH) are recognized
	when escape mode is enabled; unknown escapes are an error.

	Options:

	*-d*,*--key-delay* _<ms>_
		Delay time between key events (up/down each). Default 20ms.

	*-H*,*--key-hold* _<ms>_
		Hold each key for _<ms>_ milliseconds. Default 20ms.

	*-D*,*--next-delay* _<ms>_
		Delay between strings. Default 0ms.

	*-l*,*--layout* _<name>_
		Keyboard layout to type with. Available layouts: *us*, *de*, *fr*,
		*dvorak*, *colemak*. Default *us*. Characters that require dead keys
		are skipped with a warning.

	*-e*,*--escape* _<0|1>_
		Enable or disable escape sequences. Enabled by default for command line
		arguments, disabled by default for files and stdin.

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

# NATURAL LANGUAGE COMMANDS

*do* [*-n*,*--dry-run*] [*-j*,*--json*] [*-c*,*--min-confidence* _<0-1>_] [*-f*,*--force*] [*-l*,*--layout* _<name>_] [*--model* _<name>_] [*--endpoint* _<url>_] _<request>_

	Interpret a natural-language request with TypeSafe and perform the resulting
	input action. This command is only available when ydotool is built with
	*-DENABLE_TYPESAFE=ON*.

	The request text is sent to the TypeSafe System One API for interpretation,
	so this command needs network access and the *TYPESAFE_API_KEY* environment
	variable. Nothing else is sent, and the API key is never logged.

	The model classifies the request and the code validates and performs it.
	Recognized actions are keyboard combinations, standard application
	shortcuts (copy, paste, undo, ...), mouse clicks, wheel scrolling, pointer
	movement, and typing literal text. Requests that describe several sequential
	actions are refused: ydotool performs one action per invocation, and chaining
	is the shell's job.

	Options:

	*-n*,*--dry-run*
		Print the resolved action as JSON and exit without sending events.

	*-j*,*--json*
		Print the raw TypeSafe response.

	*-c*,*--min-confidence* _<0-1>_
		Minimum confidence required to act. Default 0.5.

	*-f*,*--force*
		Act even when confidence is below the threshold.

	*-l*,*--layout* _<name>_
		Keyboard layout used by type actions. Default *us*.

	*--model* _<name>_
		TypeSafe model. Default *jev-latest*.

	*--endpoint* _<url>_
		TypeSafe System One endpoint.

	Examples:

		ydotool do press ctrl+alt+t
		ydotool do "copy the selected text"
		ydotool do "scroll down three notches"
		ydotool do --dry-run "double-click with the left mouse button"

# MCP SERVER

*mcp* [*-n*,*--dry-run*] [*--read-only*] [*--require-focus*] [*--allow-dangerous*] [*--rate-limit* _<n>_] [*--perception* _<gnome|none>_] [*--layout* _<name>_] [*--key-delay* _<ms>_] [*--key-hold* _<ms>_] [*--screenshot-timeout* _<s>_] [*-l*,*--log-args*]

	Run a Model Context Protocol server over stdio that exposes input control
	and, on GNOME, window/focus/screenshot perception to AI agents. Only
	available in builds with *-DENABLE_MCP=ON*, which requires *atspi-2* and
	*gio-2.0*.

	The server speaks newline-delimited JSON-RPC on stdin and stdout. All logs go
	to stderr. Tools include *computer_get_state*, *computer_screenshot*,
	*computer_focus_window*, *computer_type*, *computer_press*, *computer_key*,
	*computer_click*, *computer_scroll*, *computer_move*, and the optional
	*computer_do* when built with ENABLE_TYPESAFE.

	On GNOME Wayland the compositor does not allow programs to activate windows
	directly, so *computer_focus_window* cycles windows and waits for
	accessibility events to confirm the result. Screenshots go through the
	xdg-desktop-portal and the desktop asks the user to allow each capture.

	Options:

	*-n*,*--dry-run*
		Report planned actions without sending events.

	*--read-only*
		Expose perception tools only.

	*--require-focus*
		Refuse actions when the active window is unknown.

	*--allow-dangerous*
		Allow ctrl+alt+delete, VT switches and magic sysrq combinations.

	*--rate-limit* _<n>_
		Maximum actions per second. Default 20.

	*--perception* _<gnome|none>_
		Perception backend. Default *gnome*; *none* leaves actions only.

	*--layout* _<name>_
		Keyboard layout used by *computer_type*. Default *us*.

	*--key-delay* _<ms>_
		Delay between keys. Default 20ms.

	*--key-hold* _<ms>_
		Key hold time. Default 20ms.

	*--screenshot-timeout* _<s>_
		Seconds to wait for the screenshot permission dialog. Default 60.

	*-l*,*--log-args*
		Log tool arguments to stderr. Typed text is redacted by default.

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
