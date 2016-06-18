/*
 * This file provides declarations for the extenal interface to WIDB,
 * the Windows ICI Debugger.
 *
 * Documentation of functions (where I've bothered at all) is with the
 * function definition, you've got grep dontcha?
 *
 *
 * Building
 * --------
 * To include WIDB you must change your makefile:
 *  - remove idb2.c from your project,
 *  - add the .c files in the widb subdirectory in its place,
 *  - include widb.rc in your program's resources, and
 *  - link in comctl32.lib.
 *
 * How to Use
 * ----------
 *  Insert a line
 *
 *   debug(1);
 *
 *  at the top of your ICI source file to turn debugging on.
 *
 *  The debugger will automatically pop up when exceptions occur, and you
 *  can force it to stop at a particular line (from where you can step in,
 *  out, over, etc) by adding a line
 *
 *   debug_break();
 *
 *
 * Threads
 * -------
 *  The user interface to WIDB runs in a separate thread, this is so that the
 *  ICI thread can be completely blocked while stopped in the debugger.
 *  If they ran in the same thread then
 *  dispatching messages for the WIDB interface would also dispatch messages
 *  in the application's interface.
 *
 *
 * Compiler
 * --------
 *  I've developed this using Visual C++ because that matches what
 *  I use at work.  I would be happy if it also worked on other Win32
 *  C/C++ compilers, so if you find that any modifications are necessary
 *  then please e-mail me (Oliver Bock): oliver@g7.org.
 *
 *
 * What's Missing
 * --------------
 * - Breakpoints and watch points.
 *
 * - Watch variables.
 *
 * - A pleasant interface (by this I mean more economical for debugging
 *   purposes, not adding a button bar).
 *
 *
 * Known Problems
 * --------------
 * - When the debugger is running code as it's being parsed (ie. top-level code,
 *   not in functions) it seems to get a little confused and jump out of the
 *   code instead of stepping over.  Step in (F11) to avoid this.  I haven't
 *   even looked at this problem, please tell me if it's annoying you.
 *
 * - The scope shown when the debugger intercepts an exception is wrong.
 */

#ifndef WIDB_H
#define WIDB_H

#ifdef __cplusplus
    extern "C" {
#endif

void WIDB_set_dialog_parent(HWND dialog_parent);
void WIDB_set_resources(HINSTANCE resources);
void WIDB_view_object(ici_obj_t *p, HWND parent);
void WIDB_enable_profiling_display();

#ifdef __cplusplus
    }
#endif

#endif
