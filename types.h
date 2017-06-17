// -*- mode:c++ -*-

#ifndef ICI_TYPES_H
#define ICI_TYPES_H

#include "object.h"
#include "archive.h"
#include "array.h"
#include "catch.h"
#include "channel.h"
#include "cfunc.h"
#include "exec.h"
#include "forall.h"
#include "func.h"
#include "int.h"
#include "file.h"
#include "float.h"
#include "handle.h"
#include "op.h"
#include "parse.h"
#include "profile.h"
#include "ptr.h"
#include "mark.h"
#include "mem.h"
#include "method.h"
#include "pc.h"
#include "re.h"
#include "restorer.h"
#include "saver.h"
#include "set.h"
#include "src.h"
#include "str.h"
#include "struct.h"

namespace ici
{

class archive_type : public type
{
public:
    archive_type() : type("archive", sizeof (struct ici_archive)) {}
    unsigned long    mark(ici_obj_t *o) override;
};

class array_type : public type
{
 public:
    array_type() : type("array", sizeof (struct array), type::has_forall) {}

    unsigned long       mark(ici_obj_t *o) override;
    void                free(ici_obj_t *o) override;
    unsigned long       hash(ici_obj_t *o) override;
    int                 cmp(ici_obj_t *o1, ici_obj_t *o2) override;
    ici_obj_t *         copy(ici_obj_t *o) override;
    int                 assign(ici_obj_t *o, ici_obj_t *k, ici_obj_t *v) override;
    ici_obj_t *         fetch(ici_obj_t *o, ici_obj_t *k) override;
    int                 forall(ici_obj_t *o) override;
};

class catch_type : public type
{
public:
    catch_type() : type("catch", sizeof (struct catcher)) {}

    unsigned long       mark(ici_obj_t *o) override;
    void                free(ici_obj_t *o) override;
};

class cfunc_type : public type
{
public:
    cfunc_type() : type("func", sizeof (cfunc), type::has_objname | type::has_call) {}

    unsigned long       mark(ici_obj_t *o) override;
    ici_obj_t *         fetch(ici_obj_t *o, ici_obj_t *k) override;
    void                objname(ici_obj_t *o, char p[ICI_OBJNAMEZ]) override;
    int                 call(ici_obj_t *o, ici_obj_t *subject) override;
};

class channel_type : public type
{
public:
    channel_type() : type("channel", sizeof (struct ici_channel)) {}
    unsigned long mark(ici_obj_t *o) override;
};

class exec_type : public type
{
public:
    exec_type() : type("exec", sizeof (struct exec)) {}

    unsigned long       mark(ici_obj_t *o) override;
    void                free(ici_obj_t *o) override;
    ici_obj_t *         fetch(ici_obj_t *o, ici_obj_t *k) override;
};

class file_type : public type
{
public:
    file_type() : type("file", sizeof (struct file)) {}

    unsigned long       mark(ici_obj_t *o) override;
    void                free(ici_obj_t *o) override;
    int                 cmp(ici_obj_t *o1, ici_obj_t *o2) override;
    ici_obj_t *         fetch(ici_obj_t *o, ici_obj_t *k) override;
};

class float_type : public type
{
public:
    float_type() : type("float", sizeof (struct ici_float)) {}

    unsigned long       mark(ici_obj_t *o) override;
    int                 cmp(ici_obj_t *o1, ici_obj_t *o2) override;
    unsigned long       hash(ici_obj_t *o) override;
};

class forall_type : public type
{
public:
    forall_type() : type("forall", sizeof (struct forall)) {}
    unsigned long       mark(ici_obj_t *o) override;
};

class func_type : public type
{
public:
    func_type() : type("func", sizeof (struct func), type::has_objname | type::has_call) {}

    unsigned long       mark(ici_obj_t *o) override;
    int                 cmp(ici_obj_t *o1, ici_obj_t *o2) override;
    unsigned long       hash(ici_obj_t *o) override;
    ici_obj_t *         fetch(ici_obj_t *o, ici_obj_t *k) override;
    void                objname(ici_obj_t *o, char p[ICI_OBJNAMEZ]) override;
    int                 call(ici_obj_t *o, ici_obj_t *subject) override;
};

class handle_type : public type
{
public:
    handle_type() : type("handle", sizeof (struct handle), type::has_objname) {}

    unsigned long       mark(ici_obj_t *o) override;
    void                free(ici_obj_t *o) override;

    unsigned long       hash(ici_obj_t *o) override;
    int                 cmp(ici_obj_t *o1, ici_obj_t *o2) override;
    ici_obj_t *         fetch(ici_obj_t *o, ici_obj_t *k) override;
    int                 fetch_super(ici_obj_t *o, ici_obj_t *k, ici_obj_t **v, ici_struct_t *b) override;
    ici_obj_t *         fetch_base(ici_obj_t *o, ici_obj_t *k) override;
    int                 assign_base(ici_obj_t *o, ici_obj_t *k, ici_obj_t *v) override;
    int                 assign(ici_obj_t *o, ici_obj_t *k, ici_obj_t *v) override;
    int                 assign_super(ici_obj_t *o, ici_obj_t *k, ici_obj_t *v, ici_struct_t *b) override;
    void                objname(ici_obj_t *o, char p[ICI_OBJNAMEZ]) override;
};

class int_type : public type
{
public:
    int_type() : type("int", sizeof (struct ici_int)) {}

    unsigned long       mark(ici_obj_t *o) override;
    int                 cmp(ici_obj_t *o1, ici_obj_t *o2) override;
    unsigned long       hash(ici_obj_t *o) override;
};

class mark_type : public type
{
public:
    mark_type() : type("mark", sizeof (struct mark)) {}

    unsigned long       mark(ici_obj_t *o) override;
    void                free(ici_obj_t *) override;
};

class mem_type : public type
{
public:
    mem_type() : type("mem", sizeof (struct mem)) {}

    unsigned long       mark(ici_obj_t *o) override;
    void                free(ici_obj_t *o) override;

    int                 cmp(ici_obj_t *o1, ici_obj_t *o2) override;
    unsigned long       hash(ici_obj_t *o) override;
    int                 assign(ici_obj_t *o, ici_obj_t *k, ici_obj_t *v) override;
    ici_obj_t *         fetch(ici_obj_t *o, ici_obj_t *k) override;
};

class method_type : public type
{
public:
    method_type() : type("method", sizeof (struct method), type::has_objname|type::has_call) {}

    unsigned long       mark(ici_obj_t *o) override;
    ici_obj_t *         fetch(ici_obj_t *o, ici_obj_t *k) override;
    int                 call(ici_obj_t *o, ici_obj_t *subject) override;
    void                objname(ici_obj_t *o, char p[ICI_OBJNAMEZ]) override;
};

class null_type : public type
{
public:
    null_type() : type("NULL", sizeof (struct null)) {}

    unsigned long       mark(ici_obj_t *o) override;
    void                free(ici_obj_t *o) override;
};

class op_type : public type
{
public:
    op_type() : type("op", sizeof (struct op)) {}

    unsigned long       mark(ici_obj_t *o) override;
    int                 cmp(ici_obj_t *o1, ici_obj_t *o2) override;
    unsigned long       hash(ici_obj_t *o) override;
};

class parse_type : public type
{
public:
    parse_type() : type("parse", sizeof (struct parse)) {}

    unsigned long       mark(ici_obj_t *o) override;
    void                free(ici_obj_t *o) override;
};

class pc_type : public type
{
public:
    pc_type() : type("pc", sizeof (struct pc)) {}
    unsigned long       mark(ici_obj_t *o) override;
};

class profilecall_type : public type
{
public:
    profilecall_type() : type("profile call", sizeof (struct ici_profilecall)) {}
    unsigned long       mark(ici_obj_t *o) override;
};

class ptr_type : public type
{
public:
    ptr_type() : type("ptr", sizeof (struct ptr), type::has_call) {}

    unsigned long       mark(ici_obj_t *o) override;
    int                 cmp(ici_obj_t *o1, ici_obj_t *o2) override;
    unsigned long       hash(ici_obj_t *o) override;
    ici_obj_t *         fetch(ici_obj_t *o, ici_obj_t *k) override;
    int                 assign(ici_obj_t *o, ici_obj_t *k, ici_obj_t *v) override;
    int                 call(ici_obj_t *o, ici_obj_t *subject) override;
};

class regexp_type : public type
{
public:
    regexp_type() : type("regexp", sizeof (struct regexp)) {}

    unsigned long       mark(ici_obj_t *o) override;
    void                free(ici_obj_t *o) override;

    unsigned long       hash(ici_obj_t *o) override;
    int                 cmp(ici_obj_t *o1, ici_obj_t *o2) override;
    ici_obj_t *         fetch(ici_obj_t *o, ici_obj_t *k) override;
};

class restorer_type : public type
{
public:
    restorer_type() : type("restorer", sizeof (struct restorer)) {}
    unsigned long       mark(ici_obj_t *o) override;
};

class saver_type : public type
{
public:
    saver_type() : type("saver", sizeof (struct saver)) {}
    unsigned long       mark(ici_obj_t *o) override;
};

class set_type : public type
{
public:
    set_type() : type("set", sizeof (struct set), type::has_forall) {}

    unsigned long       mark(ici_obj_t *o) override;
    void                free(ici_obj_t *o) override;

    int                 cmp(ici_obj_t *o1, ici_obj_t *o2) override;
    unsigned long       hash(ici_obj_t *o) override;
    ici_obj_t *         copy(ici_obj_t *o) override;
    int                 assign(ici_obj_t *o, ici_obj_t *k, ici_obj_t *v) override;
    ici_obj_t *         fetch(ici_obj_t *o, ici_obj_t *k) override;
    int                 forall(ici_obj_t *o) override;
};

class src_type : public type
{
public:
    src_type() : type("src", sizeof (struct src)) {}
    unsigned long       mark(ici_obj_t *o) override;
};

class string_type : public type
{
public:
    string_type() : type("string", sizeof (struct str), type::has_forall) {}

    unsigned long       mark(ici_obj_t *o) override;
    void                free(ici_obj_t *o) override;

    int                 cmp(ici_obj_t *o1, ici_obj_t *o2) override;
    ici_obj_t *         copy(ici_obj_t *o) override;
    unsigned long       hash(ici_obj_t *o) override;
    ici_obj_t *         fetch(ici_obj_t *o, ici_obj_t *k) override;
    int                 assign(ici_obj_t *o, ici_obj_t *k, ici_obj_t *v) override;
    int                 forall(ici_obj_t *o) override;
};

class struct_type : public type
{
public:
    struct_type() : type("struct", sizeof (struct ici_struct), type::has_forall) {}

    unsigned long       mark(ici_obj_t *o) override;
    void                free(ici_obj_t *o) override;

    unsigned long       hash(ici_obj_t *o) override;
    int                 cmp(ici_obj_t *o1, ici_obj_t *o2) override;
    ici_obj_t *         copy(ici_obj_t *o) override;
    int                 assign_super(ici_obj_t *o, ici_obj_t *k, ici_obj_t *v, ici_struct_t *b) override;
    int                 assign(ici_obj_t *o, ici_obj_t *k, ici_obj_t *v) override;
    int                 assign_base(ici_obj_t *o, ici_obj_t *k, ici_obj_t *v) override;
    int                 forall(ici_obj_t *o) override;
    ici_obj_t *         fetch(ici_obj_t *o, ici_obj_t *k) override;
    ici_obj_t *         fetch_base(ici_obj_t *o, ici_obj_t *k) override;
    int                 fetch_super(ici_obj_t *o, ici_obj_t *k, ici_obj_t **pv, ici_struct_t *b) override;
};

} // namespace ici

#endif
