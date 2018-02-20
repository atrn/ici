// -*- mode:c++ -*-

#ifndef ICI_REPL_H
#define ICI_REPL_H

#include "file.h"

namespace ici {

class repl_ftype;

extern class repl_ftype *repl_ftype;

bool is_repl_file(file *);

/* Inform the REPL a new statement is being parsed. */
void repl_file_new_statement(void *);

}

#endif
