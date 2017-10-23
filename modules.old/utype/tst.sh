#!/bin/sh
LD_LIBRARY_PATH=".:$LD_LIBRARY_PATH"
export LD_LIBRARY_PATH
for f in bsd linux sun sol2
do
  echo $f
  [ -x ../ici.$f ] && { ../ici.$f Utype.tst; exit; }
done
