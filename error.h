// -*- mode:c++ -*-

#ifndef ICI_ERROR_H
#define ICI_ERROR_H

#include "object.h"

namespace ici
{

/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */

/*
 * This --const-- forms part of the --ici-api--.
 */
constexpr int max_error_msg = 1024;

/*
 * Clear the ici error value, setting it to the nullptr.
 *
 * This --func-- forms part of the --ici-api--.
 */
void clear_error();

/*
 * Retrieve the ici error value. If no error is in effect
 * this returns nullptr.
 *
 * This --func-- forms part of the --ici-api--.
 */
const char *get_error();

/*
 * Set the ici error value and return non-zero as per
 * the error returning convention.
 *
 * This --func-- forms part of the --ici-api--.
 */
int set_errorc(const char *);

/*
 * Set the ici error value to the result of formatting a string with
 * the format string and other arguments, and return non-zero as per
 * the error returning convention.
 *
 * This --func-- forms part of the --ici-api--.
 */
int set_error(const char *, ...);

/*
 * Set the ici::error value by formatting a string using vsprintf
 * and return non-zero as per the error returning convention.
 *
 * This --func-- forms part of the --ici-api--.
 */
int set_errorv(const char *, va_list);

/*
 * End of ici.h export. --ici.h-end--
 */

} // namespace ici

#endif /* ICI_ERROR_H */
