// -*- mode:c++ -*-

#ifndef ICI_DEBUGGER_H
#define ICI_DEBUGGER_H

#include "object.h"

namespace ici
{

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */

/*
 * ICI debugger interface.
 *
 * A debugger is an instance of the 'ici::debugger' class, a base
 * class for ICI debuggers. ici::debugger declares a number of virtual
 * member functions called to report events of interest to debuggers.
 *
 * The interpreter has a global flag 'ici::debug_enabled' and a global
 * pointer to the debugger instance, 'ici::debugger'. If the flag is
 * set, the interpreter calls the debugger's member functions at the
 * appropriate times.  See 'ici::debugger' and 'ici::debug_enabled'.
 *
 * The ici::debugger base-class provides default implementations of
 * the debugging functions. The default implementations do nothing.
 *
 * error_set()          Called when an error is being set, just prior to
 *                      the assignment to ICI's 'error' variable. This is
 *                      supplied the error string that will be set and
 *                      is called regardless of the error being handled
 *                      by an 'onerror' block.
 *
 * error_uncaught()     Called upon an uncaught error. This is is called
 *                      AFTER the stack has been unwound and no handler
 *                      was found.
 *
 * function_call()      Called with the object being called, the pointer to
 *                      the first actual argument (see 'ARGS()' and the number
 *                      of actual arguments just before control is transfered
 *                      to a callable object (function, method or anything
 *                      else).
 *
 * function_result()    Called with the object being returned from any call.
 *
 * source_line()        Called each time execution passes into the region of a
 *                      new source line marker.  These typically occur before
 *                      any of the code generated by a particular line of
 *                      source.
 *
 * watch()              In theory, called when assignments are made.  However
 *                      optimisations in the interpreter have made this
 *                      difficult to support without performance penalties
 *                      even when debugging is not enabled.  So it is
 *                      currently disabled.  The function remains here pending
 *                      discovery of a method of watching things efficiently.
 *
 * This --class-- forms part of the --ici-api--.
 */
class debugger
{
public:
    virtual ~debugger();
    virtual void error_set(const char *, struct src *);
    virtual void error_uncaught(const char *, struct src *);
    virtual void function_call(object *, object **, int);
    virtual void function_result(object *);
    virtual void cfunc_call(object *);
    virtual void cfunc_result(object *);
    virtual void source_line(struct src *);
    virtual void watch(object *, object *, object *);
    virtual void finished();
};

/*
 * End of ici.h export. --ici.h-end--
 */

} // namespace ici

#endif
