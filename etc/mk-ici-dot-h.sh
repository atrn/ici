#!/bin/sh
case $(uname) in
Darwin)
    conf_dot_h=conf/darwin.h
    ;;
FreeBSD)
    conf_dot_h=conf/freebsd.h
    ;;
Linux)
    conf_dot_h=conf/linux.h
    ;;
*)
    echo $(uname) not supported 1>&2
    exit 1
    ;;
esac
cd $2
ici=ici
if [ -x "$1" ]; then
    ici="$1"
elif [ -x "$1/Debug/ici" ]; then
    ici="$1/Debug/ici"
elif [ -x "$1/Release/ici" ]; then
    ici="$1/Release/ici"
fi
"${ici}" mk-ici-h.ici ${conf_dot_h} "$2" "$3"
