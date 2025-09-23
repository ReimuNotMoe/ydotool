#compdef ydotool

_ydotool() {
    local -a commands
    commands=(click mousemove type key debug bakers stdin)

    _arguments \
        '(-h --help)'{-h,--help}'[Show help message]' \
        '1:command:_values "commands" $commands[@]'
}

_ydotool "$@"
