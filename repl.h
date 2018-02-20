// -*- mode:c++ -*-

#ifndef ICI_REPL_H
#define ICI_REPL_H

#include "ftype.h"

namespace ici {

extern ftype *repl_ftype;

/* Inform the REPL a new statement is being parsed. */
void repl_file_new_statement(void *);

}

#endif
