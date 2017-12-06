cmake_minimum_required(VERSION 3.8)
project(ici LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 11)
add_executable(
  ici
  etc/main.cc
  alloc.cc aplfuncs.cc archiver.cc arith.cc array.cc
  buf.cc
  call.cc catcher.cc cfunc.cc cfunco.cc channel.cc
  compile.cc conf.cc control.cc crc.cc crc32c.cc
  error.cc events.cc exec.cc exerror.cc
  file.cc findpath.cc float.cc  forall.cc ftype.cc func.cc
  handle.cc hash.cc
  icimain.cc idb.cc idb2.cc init.cc int.cc
  lex.cc load.cc
  map.cc mark.cc mem.cc method.cc mkvar.cc
  net.cc null.cc
  object.cc oofuncs.cc op.cc
  parse.cc pc.cc pcre.cc profile.cc ptr.cc
  refuncs.cc regexp.cc
  set.cc sfile.cc signals.cc smash.cc src.cc
  sstring.cc string.cc strtol.cc study.cc
  sys.cc syserr.cc
  thread.cc type.cc types.cc
  unary.cc uninit.cc
  win32err.cc win32timezone.cc
  )
