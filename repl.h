// -*- mode:c++ -*-

#ifndef ICI_REPL_H
#define ICI_REPL_H

#include "file.h"

namespace ici {

bool is_repl_file(file *);

/* Inform the REPL a new statement is being parsed. */
void repl_file_new_statement(void *, bool sol);

}

#endif
