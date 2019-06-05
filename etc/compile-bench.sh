#!/bin/sh

eval `echo $*`

S=${S:-4}
E=${E:-16}
R=${R:-1}
I=${I:-2}

echo '"# jobs", "real", "total cpu", "user cpu", "sys cpu", "speedup"'
for njobs in `seq $S $I $E`; do
  if [ $njobs = 0 ]; then continue; fi
  for attempt in `seq $R`; do
    make -s clean
    (time make build=exe dccflags="--quiet -j${njobs}") 2>&1 | (
	printf "%d, " $njobs
	awk '
NR > 1 {
    sub(/^0m/, "", $2)
    sub(/s$/, "", $2)
    a[$1] = $2
}
END {
    r=a["real"]
    u=a["user"]
    s=a["sys"]
    t=u+s
    f=t/r
    print r ", " t ", " u ", " s ", " f
}
'
    )
  done
done
