ydotoold(8)

# NAME

ydotoold \- daemon for *ydotool*(1)

# SYNOPSIS

*ydotoold* _[OPTION...]_

# DESCRIPTION

*ydotoold* holds a persistent virtual device, and accepts input from *ydotool*(1).

# OPTIONS

	*-p*, *--socket-path arg* _<path>_
		Set socket path.

	*-P*, *--socket-perm arg* _<perms>_
		Set socket permission.

	*-o*, *--socket-own=UID:GID*
	    Socket ownership.

	*-m*, *--mouse-off*
	    Disable mouse (EV_REL)

	*-k*, *--keyboard-off*
	    Disable keyboard (EV_KEY)
		
	*-T*, *--touch-on*
        Enable touchscreen (EV_ABS)

	*-h*, *--help*
		Display help and exit.
	
	*-V*, *--version*
		Show version information.

# AUTHOR

*ydotool*(1) and *ydotoold*(8) were written by ReimuNotMoe.

This manpage was written by bob.hepple@gmail.com but updated since.

# LICENCE

AGPLv3

# SEE ALSO

*ydotool*(1)

Project site: <https://github.com/ReimuNotMoe/ydotool>
