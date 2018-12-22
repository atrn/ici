/*
 *  'icistr.h' files are used by ICI's 'icistr-setup.h' header file to
 *  define 'static' ICI string objects.  Basically if a module has any
 *  known strings they are defined in an 'icistr.h' file.
 *
 *  The names of a module's functions and other exported values
 *  are ICI strings so they typically appear in this file and
 *  are used to initialize the module's cfunc table.
 */

ICI_STR(function, "function")
ICI_STR(other_function, "other_function")
