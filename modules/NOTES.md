# Build-related Environment Variables

- ICI_MODULE_NAME  
The, short, name of the module being built.

- ICI_DEBUG_BUILD  
Defined when doing a debug (!NDEBUG) build.

- ICI_BUILD_TYPE_DLL  
Defined when ici is built as a dynamic library.

- ICI_MACOS_BUNDLE_HOST  
The name of the _bundle host_. Either `libici.dylib` when ici is built
as a dynamic library or `ici` when built as a single executable.

- ICI_DOT_H_DIR  
The absolute pathname of the directory where `ici.h` is
located. Either the name of the ici source directory or the name of
the install directory (${PREFIX}/include)

- ICI_LIB_DIR  
The absolute pathname of the directory where the ici library
is located. Either the name of the ici source directory or the name of
the install directory (${PREFIX}/lib)

- ICI_PLUGIN_SUFFIX  
Filename suffix (extension) used with ICI module plugins on the target
platform.  On macOS this is ".bundle", on ELF platforms ".so" and on
Windows (when eventually supported) it is ".dll".
