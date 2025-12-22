# bash completion for ydotoold                          -*- shell-script -*-

_ydotoold_completions() {
	local cur="${COMP_WORDS[COMP_CWORD]}"
	local options="-h --help -V --version -p --socket-path -P --socket-perm -m --mouse-off -k --keyboard-off -T --touch-on"
	COMPREPLY=($(compgen -W "$options" -- "$cur"))
}

complete -F _ydotoold_completions ydotoold

# ex: ts=4 sw=4 et filetype=sh
