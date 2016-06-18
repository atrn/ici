#ifndef ICI_TRACE_H
#define ICI_TRACE_H
/*
 * The following portion of this file exports to ici.h. --ici.h-start--
 */

extern int ici_trace_yes;
extern int ici_trace_flags;

#define TRACE_LEXER         1
#define TRACE_EXPR          2
#define TRACE_INTRINSICS    4
#define TRACE_FUNCS         8
#define TRACE_MEM           16
#define TRACE_SRC           32
#define TRACE_GC            64

#define TRACE_ALL       (TRACE_EXPR|TRACE_FUNCS|TRACE_GC)
/*
 * End of ici.h export. --ici.h-end--
 */

#endif /* ICI_TRACE_H */
