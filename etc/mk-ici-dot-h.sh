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

wants_ipp=
case $(uname) in
    Darwin)
        if otool -L "${ici}" | grep -q libippcore; then
            wants_ipp=YES
        fi
        ;;
    *)
        if ldd "${ici}" | grep -q libippcore; then
            wants_ipp=YES
        fi
        ;;
esac

if [ "${wants_ipp}" = YES ]; then
    if [ -f /opt/intel/ipp/bin/ippvars.sh ]; then
        . /opt/intel/ipp/bin/ippvars.sh
    else
        echo "warning: no ippvars.sh sourced" 1>&2
    fi
fi

exec "${ici}" mk-ici-h.ici ${conf_dot_h} "$2" "$3"
