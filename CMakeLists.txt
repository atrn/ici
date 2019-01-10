#  Cmake "lists" for ICI.
#
#  By default this builds the ICI dynamic library and corresponding
#  ici.h file, and a ici executable linked against the dynamic
#  library.
#
#  Alternatively, if the ICI_BUILD_LIB option is disabled (set to OFF)
#  no library is built.
#  
#  There is no 'install' support as yet.
#

cmake_minimum_required(VERSION 3.0 FATAL_ERROR) # A guess

project(ici LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 11)

option(ICI_BUILD_LIB "Build the ICI shared object" ON)

set(ICI_MAIN etc/main.cc)

set(ICI_SOURCES
  alloc.cc
  aplfuncs.cc
  archiver.cc
  arith.cc
  array.cc
  buf.cc
  call.cc
  catcher.cc
  cfunc.cc
  cfunco.cc
  channel.cc
  compile.cc
  conf.cc
  control.cc
  crc.cc
  crc32c.cc
  debug.cc
  debugger.cc
  error.cc
  events.cc
  exec.cc
  exerror.cc
  file.cc
  findpath.cc
  float.cc
  forall.cc
  ftype.cc
  func.cc
  handle.cc
  icimain.cc
  init.cc
  int.cc
  intf.cc
  lex.cc
  load.cc
  map.cc
  mark.cc
  mem.cc
  method.cc
  mkvar.cc
  net.cc
  null.cc
  object.cc
  oofuncs.cc
  op.cc
  parse.cc
  pc.cc
  pcre.cc
  profile.cc
  ptr.cc
  refuncs.cc
  regexp.cc
  repl.cc
  set.cc
  sfile.cc
  signals.cc
  smash.cc
  src.cc
  sstring.cc
  string.cc
  strtol.cc
  study.cc
  sys.cc
  syserr.cc
  thread.cc
  type.cc
  types.cc
  unary.cc
  uninit.cc
  win32err.cc
  win32timezone.cc
  alloc.h
  archiver.h
  array.h
  binop.h
  buf.h
  catcher.h
  cfunc.h
  channel.h
  chartables.i
  debugger.h
  error.h
  exec.h
  file.h
  float.h
  forall.h
  ftype.h
  func.h
  fwd.h
  handle.h
  icistr-setup.h
  int.h
  intf.h
  internal.h
  load-w32.h
  map.h
  mark.h
  mem.h
  method.h
  null.h
  object.h
  op.h
  parse.h
  pc.h
  pcre.h
  primes.h
  profile.h
  ptr.h
  re.h
  repl.h
  set.h
  src.h
  sstring.h
  str.h
  type.h
  types.h
  wrap.h)


if (ICI_BUILD_LIB)
  add_library(ici SHARED ${ICI_SOURCES})
  add_executable(ici_exe ${ICI_MAIN})
  target_link_libraries(ici_exe PUBLIC ici)
else()
  add_executable(ici_exe ${ICI_MAIN} ${ICI_SOURCES})
endif()

#  The executable target can't be called 'ici' as that's the library
#  target name.  Instead we call it 'ici_exe'. But that results in the
#  executable being called 'ici_exe[.exe]' so we have to set the
#  'OUTPUT_NAME' property to get the file name we want.
#
if(WIN32)
  set_target_properties(ici_exe PROPERTIES OUTPUT_NAME ici.exe)
else()
  set_target_properties(ici_exe PROPERTIES OUTPUT_NAME ici)
endif()

#  Required libraries:
#
#  - Linux and FreeBSD need -lpthread
#  - Linux still has dlopen et al in libdl
#
if("${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "FreeBSD")
  target_link_libraries(ici_exe PUBLIC -lpthread)
endif()
if("${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "Linux")
  target_link_libraries(ici_exe PUBLIC -ldl -lpthread)
endif()

#  To create ici.h we use the, just built, ici executable to
#  run mk-ici-h.ici which creates ici.h from the ICI headers.
#  To run that specific executable we need to use the actual
#  OUTPUT_NAME property of ici_exe.
#
#  Also mk-ici-h.ici needs to be told the specific config
#  header file used so we figure that out too (fwd.h does
#  something similar).
#
get_target_property(ici_cmd ici_exe OUTPUT_NAME)
if(WIN32)
  set(CONF "conf/w32.h")
endif()
if(APPLE)
  set(CONF "conf/darwin.h")
endif()
if("${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "FreeBSD")
  set(CONF, "conf/freebsd.h")
endif()
if("${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "Linux")
  set(CONF, "conf/linux.h")
endif()

add_custom_command(TARGET ici_exe POST_BUILD
  COMMENT "[----] Creating ici.h"
  COMMAND ${ici_cmd}
              ${CMAKE_CURRENT_SOURCE_DIR}/mk-ici-h.ici
              ${CONF}
              ${CMAKE_CURRENT_SOURCE_DIR}
              ${CMAKE_CURRENT_BINARY_DIR}
)
