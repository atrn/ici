#  Cmake "lists" for ICI.
#
#  By default this builds the ICI dynamic library and corresponding
#  ici.h file, and a ici executable linked against the dynamic
#  library.
#
#  Alternatively, if the ICI_BUILD_SHARED_LIB option is disabled
#  (set to OFF) no library is built.
#  
#  There is no 'install' support as yet.
#

cmake_minimum_required(VERSION 3.0 FATAL_ERROR) # A guess

project(ici LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 14)

option(ICI_BUILD_DYNAMIC_LIB    "Build the ICI dynamic library"         ON)
option(ICI_BUILD_STATIC_LIB     "Build the ICI static library"          OFF)
option(ICI_VEC_USE_IPP          "Use Intel IPP for vec arithmetic"      ON)

# Use IPP_ROOT environment variable to find IPP
cmake_policy(SET CMP0074 NEW)

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
  ref.h
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
  userop.cc
  vec.cc
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
  vec.h
  wrap.h
  userop.h)

if(WIN32)
  set(ICI_SOURCES "${ICI_SOURCES} win32err.cc win32timezone.cc")
endif()

# Explicitly disable any optimization for debug builds (some compilers
# optimize by default, e.g. icc)
#
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g" CACHE STRING "" FORCE)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#  If we're using IPP we need to "find" it. To do that we have a
#  custom cmake module, FindIPP.cmake. To locate FindIPP.cmake cmake
#  uses a _module path_ and we add our 'etc' directory to that path as
#  our file resides there.  That's not enough.
#
#  To allow rebuilds to work we have to, (a) force the modified value
#  of CMAKE_MODULE_PATH to be set in cmake's cache file and, (b) use
#  an absolute path for our 'etc' directory so it can be found.
#
if(ICI_VEC_USE_IPP)
  set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/etc" CACHE PATH "" FORCE)
  find_package(IPP REQUIRED)
  cmake_policy(SET CMP0042 NEW)
  set(CMAKE_BUILD_RPATH ${IPP_LIBRARIES_PATH})
endif(ICI_VEC_USE_IPP)


#  Building...
#
if(ICI_BUILD_DYNAMIC_LIB)
  set(ICI_TARGET "ici")
  add_library(ici SHARED ${ICI_SOURCES})
  add_executable(ici_exe etc/main.cc)
  target_link_libraries(ici_exe PUBLIC ici)
elseif(ICI_BUILD_STATIC_LIB)
  set(ICI_TARGET "ici_exe")
  add_library(ici STATIC ${ICI_SOURCES})
  add_executable(ici_exe etc/main.cc)
  target_link_libraries(ici_exe PUBLIC ici)
else()
  set(ICI_TARGET "ici_exe")
  add_executable(ici_exe etc/main.cc ${ICI_SOURCES})
endif()


#  The executable target can't be called 'ici' since that's the name
#  of the library.  Instead we call it 'ici_exe'. Doing that, however,
#  results in the executable file being named 'ici_exe[.exe]' which we
#  don't want so we have to set the 'OUTPUT_NAME' property of the
#  ici_exe target to the file name we want.
#
if(WIN32)
  set_target_properties(ici_exe PROPERTIES OUTPUT_NAME ici.exe)
else()
  set_target_properties(ici_exe PROPERTIES OUTPUT_NAME ici)
endif()

#  Required libraries:
#
#  - Linux and FreeBSD need -lpthread
#  - And Linux glibc still has dlopen et al in libdl
#
if("${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "FreeBSD")
  target_link_libraries(ici_exe PUBLIC -lpthread)
endif()
if ("${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "Linux")
  target_link_libraries(ici_exe PUBLIC -ldl -lpthread)
endif()

#  And if we're using IPP we need to include its options
#  and libraries.
#
if(ICI_VEC_USE_IPP)
  target_compile_definitions(${ICI_TARGET} PUBLIC ICI_VEC_USE_IPP)
  target_include_directories(${ICI_TARGET} PUBLIC ${IPP_INCLUDE_DIRS})
  target_link_libraries(${ICI_TARGET} PUBLIC ${IPP_LIBRARIES})
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
add_custom_command(TARGET ici_exe POST_BUILD
  COMMENT "[----] Creating ici.h"
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/etc/mk-ici-dot-h.sh
      ${ici_BINARY_DIR}/ici ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
)
