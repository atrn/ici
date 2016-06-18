#ifndef ICI_BUF_H
#define ICI_BUF_H
/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */
/*
 * Ensure that 'ici_buf' points to enough memory to hold index 'n' (plus
 * room for a nul char at the end). Returns 0 on success, else 1 and
 * sets 'ici_error'.
 *
 * See also: 'The error return convention'.
 *
 * This --macro-- forms part of the --ici-api--.
 */
#define ici_chkbuf(n)       (ici_bufz > (int)(n) ? 0 : ici_growbuf(n))

extern int      ici_growbuf(int);

/*
 * End of ici.h export. --ici.h-end--
 */

/*
 * We use buf as an abbreviation for ici_buf in the core.
 */
#define buf     ici_buf

#endif /* ICI_BUF_H */
