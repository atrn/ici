// -*- mode:c++ -*-

#ifndef ICI_TYPES_H
#define ICI_TYPES_H

#include "object.h"

namespace ici
{

class archive_type : public type
{
public:
    archive_type() : type("archive") {}

    virtual unsigned long mark(ici_obj_t *o) override;
    virtual void free(ici_obj_t *o) override;
};

class array_type : public type
{
 public:
    array_type() : type("array") {}

    virtual unsigned long mark(ici_obj_t *o) override;
    virtual void free(ici_obj_t *o) override;
    virtual unsigned long hash(ici_obj_t *o) override;
    virtual int cmp(ici_obj_t *o1, ici_obj_t *o2) override;
    virtual ici_obj_t * copy(ici_obj_t *o) override;
    virtual int assign(ici_obj_t *o, ici_obj_t *k, ici_obj_t *v) override;
    virtual ici_obj_t *fetch(ici_obj_t *o, ici_obj_t *k) override;
    virtual bool has_forall() const override { return true; }
    virtual int forall(ici_obj_t *o) override;
};

class catch_type : public type
{
public:
    catch_type() : type("catch") {}

    virtual unsigned long mark(ici_obj_t *o) override;
    virtual void free(ici_obj_t *o) override;
};

class cfunc_type : public type
{
public:
    cfunc_type() : type("func") {}

    bool has_objname() const override { return true; }
    bool has_call() const override { return true; }

    virtual unsigned long mark(ici_obj_t *o) override;
    virtual void free(ici_obj_t *o) override;
    virtual ici_obj_t * fetch(ici_obj_t *o, ici_obj_t *k) override;
    virtual void objname(ici_obj_t *o, char p[ICI_OBJNAMEZ]) override;
    virtual int call(ici_obj_t *o, ici_obj_t *subject) override;
};

class channel_type : public type
{
public:
    channel_type() : type("channel") {}

    unsigned long mark(ici_obj_t *o) override;
    void free(ici_obj_t *o) override;
};

class exec_type : public type
{
public:
    exec_type() : type("exec") {}

    unsigned long mark(ici_obj_t *o) override;
    void free(ici_obj_t *o) override;
    ici_obj_t *fetch(ici_obj_t *o, ici_obj_t *k) override;
};

class file_type : public type
{
public:
    file_type() : type("file") {}

    virtual unsigned long mark(ici_obj_t *o) override;
    virtual void free(ici_obj_t *o) override;
    virtual int cmp(ici_obj_t *o1, ici_obj_t *o2) override;
    virtual ici_obj_t * fetch(ici_obj_t *o, ici_obj_t *k) override;
};

class float_type : public type
{
public:
    float_type() : type("float") {}

    unsigned long mark(ici_obj_t *o) override;
    void free(ici_obj_t *o) override;
    int cmp(ici_obj_t *o1, ici_obj_t *o2) override;
    unsigned long hash(ici_obj_t *o) override;
};

class forall_type : public type
{
public:
    forall_type() : type("forall") {}

    unsigned long mark(ici_obj_t *o) override;
    void free(ici_obj_t *o) override;
};

class func_type : public type
{
public:
    func_type() : type("func") {}
    
    bool has_objname() const override { return true; }
    bool has_call() const override { return true; }

    unsigned long mark(ici_obj_t *o) override;
    void free(ici_obj_t *o) override;
    int cmp(ici_obj_t *o1, ici_obj_t *o2) override;
    unsigned long hash(ici_obj_t *o) override;
    ici_obj_t * fetch(ici_obj_t *o, ici_obj_t *k) override;
    void objname(ici_obj_t *o, char p[ICI_OBJNAMEZ]) override;
    int call(ici_obj_t *o, ici_obj_t *subject) override;
};

class handle_type : public type
{
public:
    handle_type() : type("handle") {}

    bool has_objname() const override { return true; }

    unsigned long mark(ici_obj_t *o) override;
    void free(ici_obj_t *o) override;

    unsigned long hash(ici_obj_t *o) override;
    int cmp(ici_obj_t *o1, ici_obj_t *o2) override;
    ici_obj_t * fetch(ici_obj_t *o, ici_obj_t *k) override;
    int fetch_super(ici_obj_t *o, ici_obj_t *k, ici_obj_t **v, ici_struct_t *b) override;
    ici_obj_t * fetch_base(ici_obj_t *o, ici_obj_t *k) override;
    int assign_base(ici_obj_t *o, ici_obj_t *k, ici_obj_t *v) override;
    int assign(ici_obj_t *o, ici_obj_t *k, ici_obj_t *v) override;
    int assign_super(ici_obj_t *o, ici_obj_t *k, ici_obj_t *v, ici_struct_t *b) override;
    void objname(ici_obj_t *o, char p[ICI_OBJNAMEZ]) override;
};

class int_type : public type
{
public:
    int_type() : type("int") {}

    unsigned long  mark(ici_obj_t *o) override;
    void free(ici_obj_t *o) override;
    int cmp(ici_obj_t *o1, ici_obj_t *o2) override;
    unsigned long hash(ici_obj_t *o) override;
};

class mark_type : public type
{
public:
    mark_type() : type("mark") {}

    unsigned long mark(ici_obj_t *o) override;
    void free(ici_obj_t *) override;
};

class mem_type : public type
{
public:
    mem_type() : type("mem") {}

    unsigned long mark(ici_obj_t *o) override;
    int cmp(ici_obj_t *o1, ici_obj_t *o2) override;
    unsigned long hash(ici_obj_t *o) override;
    void free(ici_obj_t *o) override;
    int assign(ici_obj_t *o, ici_obj_t *k, ici_obj_t *v) override;
    ici_obj_t * fetch(ici_obj_t *o, ici_obj_t *k) override;
};

class method_type : public type
{
public:
    method_type() : type("method") {}

    bool has_objname() const override { return true; }
    bool has_call() const override { return true; }

    unsigned long mark(ici_obj_t *o) override;
    void free(ici_obj_t *o) override;
    ici_obj_t * fetch(ici_obj_t *o, ici_obj_t *k) override;
    int call(ici_obj_t *o, ici_obj_t *subject) override;
    void objname(ici_obj_t *o, char p[ICI_OBJNAMEZ]) override;
};

class null_type : public type
{
public:
    null_type() : type("null") {}

    unsigned long mark(ici_obj_t *o) override;
    void free(ici_obj_t *o) override;
};

class op_type : public type
{
public:
    op_type() : type("op") {}

    unsigned long mark(ici_obj_t *o) override;
    void free(ici_obj_t *o) override;
    int cmp(ici_obj_t *o1, ici_obj_t *o2) override;
    unsigned long hash(ici_obj_t *o) override;
};

class parse_type : public type
{
public:
    parse_type() : type("parse") {}

    unsigned long mark(ici_obj_t *o) override;
    void free(ici_obj_t *o) override;
};

class pc_type : public type
{
public:
    pc_type() : type("pc") {}

    unsigned long mark(ici_obj_t *o) override;
    void free(ici_obj_t *o) override;
};

class profilecall_type : public type
{
public:
    profilecall_type() : type("profile call") {}

    unsigned long mark(ici_obj_t *o) override;
    void free(ici_obj_t *o) override;
};

class ptr_type : public type
{
public:
    ptr_type() : type("ptr") {}

    bool has_call() const override { return true; }

    unsigned long mark(ici_obj_t *o) override;
    void free(ici_obj_t *o) override;
    int cmp(ici_obj_t *o1, ici_obj_t *o2) override;
    unsigned long hash(ici_obj_t *o) override;
    ici_obj_t * fetch(ici_obj_t *o, ici_obj_t *k) override;
    int assign(ici_obj_t *o, ici_obj_t *k, ici_obj_t *v) override;
    int call(ici_obj_t *o, ici_obj_t *subject) override;
};

class regexp_type : public type
{
public:
    regexp_type() : type("regexp") {}

    unsigned long mark(ici_obj_t *o) override;
    void free(ici_obj_t *o) override;
    unsigned long hash(ici_obj_t *o) override;
    int cmp(ici_obj_t *o1, ici_obj_t *o2) override;
    ici_obj_t *fetch(ici_obj_t *o, ici_obj_t *k) override;
};

class restorer_type : public type
{
public:
    restorer_type() : type("restorer") {}

    unsigned long mark(ici_obj_t *o) override;
    void free(ici_obj_t *o) override;
};

class saver_type : public type
{
public:
    saver_type() : type("saver") {}

    unsigned long mark(ici_obj_t *o) override;
    void free(ici_obj_t *o) override;
};

class set_type : public type
{
public:
    set_type() : type("set") {}

    bool has_forall() const override { return true;}

    unsigned long mark(ici_obj_t *o) override;
    void free(ici_obj_t *o) override;
    int cmp(ici_obj_t *o1, ici_obj_t *o2) override;
    unsigned long hash(ici_obj_t *o) override;
    ici_obj_t * copy(ici_obj_t *o) override;
    int assign(ici_obj_t *o, ici_obj_t *k, ici_obj_t *v) override;
    ici_obj_t * fetch(ici_obj_t *o, ici_obj_t *k) override;
    int forall(ici_obj_t *o) override;
};

class src_type : public type
{
public:
    src_type() : type("src") {}

    unsigned long mark(ici_obj_t *o) override;
    void free(ici_obj_t *o) override;
};

class string_type : public type
{
public:
    string_type() : type("string") {}

    bool has_forall() const override { return true; }

    unsigned long mark(ici_obj_t *o) override;
    int cmp(ici_obj_t *o1, ici_obj_t *o2) override;
    ici_obj_t *copy(ici_obj_t *o) override;
    void free(ici_obj_t *o) override;
    unsigned long hash(ici_obj_t *o) override;
    ici_obj_t *fetch(ici_obj_t *o, ici_obj_t *k) override;
    int assign(ici_obj_t *o, ici_obj_t *k, ici_obj_t *v) override;
    int forall(ici_obj_t *o) override;
};

class struct_type : public type
{
public:
    struct_type() : type("struct") {}
    bool has_forall() const override { return true; }

    unsigned long mark(ici_obj_t *o) override;
    void free(ici_obj_t *o) override;
    unsigned long hash(ici_obj_t *o) override;
    int cmp(ici_obj_t *o1, ici_obj_t *o2) override;
    ici_obj_t *copy(ici_obj_t *o) override;
    int assign_super(ici_obj_t *o, ici_obj_t *k, ici_obj_t *v, ici_struct_t *b) override;
    int assign(ici_obj_t *o, ici_obj_t *k, ici_obj_t *v) override;
    int assign_base(ici_obj_t *o, ici_obj_t *k, ici_obj_t *v) override;
    int forall(ici_obj_t *o) override;
    ici_obj_t *fetch(ici_obj_t *o, ici_obj_t *k) override;
    ici_obj_t *fetch_base(ici_obj_t *o, ici_obj_t *k) override;
    int fetch_super(ici_obj_t *o, ici_obj_t *k, ici_obj_t **pv, ici_struct_t *b) override;
};

} // namespace ici

#endif
