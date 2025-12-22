#compdef ydotoold

_ydotoold() {
    _arguments \
        '(-h --help)'{-h,--help}'[Display help and exit]' \
        '(-V --version)'{-V,--version}'[Show version information]' \
        '(-p --socket-path)'{-p,--socket-path}'[Custom socket path]:path:_files' \
        '(-P --socket-perm)'{-P,--socket-perm}'[Socket permission (default 0600)]:permission:' \
        '(-o --socket-own)'{-o,--socket-own}'[Socket ownership]:ownership:' \
        '(-m --mouse-off)'{-m,--mouse-off}'[Disable mouse (EV_REL)]' \
        '(-k --keyboard-off)'{-k,--keyboard-off}'[Disable keyboard (EV_KEY)]' \
        '(-T --touch-on)'{-T,--touch-on}'[Enable touchscreen (EV_ABS)]'
}

_ydotoold "$@"
