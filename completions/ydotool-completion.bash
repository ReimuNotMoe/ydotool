# bash completion for ydotool                          -*- shell-script -*-

_ydotool_completions() {
	local cur="${COMP_WORDS[COMP_CWORD]}"
	local options="-h --help click mousemove type key debug bakers stdin"
	COMPREPLY=($(compgen -W "$options" -- "$cur"))
}

complete -F _ydotool_completions ydotool

# ex: ts=4 sw=4 et filetype=sh
