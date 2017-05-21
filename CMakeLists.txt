cmake_minimum_required(VERSION 3.0)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_MACOS_RPATH ON)

add_library(anici SHARED
  alloc.c
  aplfuncs.c
  archive.c
  arith.c
  array.c
  buf.c
  call.c
  catch.c
  cfunc.c
  cfunco.c
  channel.c
  compile.c
  conf.c
  control.c
  crc.c
  crc32c.c
  error.c
  events.c
  exec.c
  exerror.c
  file.c
  findpath.c
  float.c
  forall.c
  func.c
  handle.c
  hash.c
  icimain.c
  idb.c
  idb2.c
  init.c
  int.c
  lex.c
  load.c
  main.c
  mark.c
  mem.c
  method.c
  mkvar.c
  null.c
  object.c
  oofuncs.c
  op.c
  parse.c
  pc.c
  profile.c
  ptr.c
  refuncs.c
  regexp.c
  restore.c
  save.c
  set.c
  sfile.c
  signals.c
  smash.c
  src.c
  sstring.c
  string.c
  strtol.c
  struct.c
  syserr.c
  thread.c
  unary.c
  uninit.c
  win32err.c
  win32timezone.c
  macos/semaphore.c
  pcre/chartables.c
  pcre/pcre.c
  pcre/study.c
)

target_compile_definitions(anici PRIVATE NDEBUG)
target_include_directories(anici PRIVATE macos)
