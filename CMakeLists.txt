project(ici LANGUAGES CXX)
cmake_minimum_required(VERSION 3.0)

#  ICI uses a few C++11 things but we'll use C++14 since
#  its a more complete C++ iteration.
#
set(CMAKE_CXX_STANDARD 14)

#  The interpreter can be built as a shared library
#  and executable that uses that library or as a
#  simple executable.
#
option(BUILD_ICI_DLL "build a DLL" ON)

#  Names of the base ici source and header files.
#
set(SRCFILES
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


# When building the DLL we create a shared library and an executable
# linked against that library.  Otherwise we just create the ici
# executable.
#
if (BUILD_ICI_DLL)

  add_library(ici SHARED ${SRCFILES})

  # The executable target can't be called 'ici' since that's the
  # library's target name.  Instead we call it 'ici_exe' and set
  # the 'OUTPUT_NAME' property to get the correct file name.
  #
  add_executable(ici_exe etc/main.cc)
  target_link_libraries(ici_exe PUBLIC ici)

  if(WIN32)
    set_target_properties(ici_exe PROPERTIES OUTPUT_NAME ici.exe)
  else()
    set_target_properties(ici_exe PROPERTIES OUTPUT_NAME ici)
  endif()

else()

  # Build ici executable directly from all sources.
  #
  add_executable(ici etc/main.cc ${SRCFILES})

endif()

# Required external libraries
#

if("${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "FreeBSD")
  target_link_libraries(ici PUBLIC -lpthread)
endif()

if("${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "Linux")
  target_link_libraries(ici PUBLIC -ldl -lpthread)
endif()
