#define VECMISMATCH() goto vecmismatch

#define MATCHVEC(VECOF)                                 \
    if (VECOF(o0)->v_size != VECOF(o1)->v_size)         \
    {                                                   \
        VECMISMATCH();                                  \
    }

#define VEC_VEC_ASSIGNOP(VEC, VECOF, BINOP, OP)         \
    case ICI_TRI(VEC, VEC, BINOP):                      \
        MATCHVEC(VECOF);                                \
        o = o0;                                         \
        (*VECOF(o)) OP (*VECOF(o1));                    \
        USEo();

#define VEC_INT_ASSIGNOP(VEC, VECOF, BINOP, OP)         \
    case ICI_TRI(VEC, TC_INT, BINOP):                   \
        o = o0;                                         \
        (*VECOF(o)) OP intof(o1)->i_value;              \
        USEo();

#define VEC_FLOAT_ASSIGNOP(VEC, VECOF, BINOP, OP)       \
    case ICI_TRI(VEC, TC_FLOAT, BINOP):                 \
        o = o0;                                         \
        (*VECOF(o)) OP floatof(o1)->f_value;            \
        USEo();

#define VEC_VEC_BINOP(VEC, VECOF, BINOP, OP)            \
    case ICI_TRI(VEC, VEC, BINOP):                      \
        MATCHVEC(VECOF);                                \
        o = o0;                                         \
        (*VECOF(o)) OP (*VECOF(o1));                    \
        USEo();

#define VEC_INT_BINOP(VEC, NEWVEC, VECOF, BINOP, OP)    \
    case ICI_TRI(VEC, TC_INT, BINOP):                   \
        if ((o = NEWVEC(VECOF(o0))) == nullptr)         \
        {                                               \
            FAIL();                                     \
        }                                               \
        (*VECOF(o)) OP intof(o1)->i_value;              \
        USEo();

#define VEC_FLOAT_BINOP(VEC, NEWVEC, VECOF, BINOP, OP)  \
    case ICI_TRI(VEC, TC_FLOAT, BINOP):                 \
        if ((o = NEWVEC(VECOF(o0))) == nullptr)         \
        {                                               \
            FAIL();                                     \
        }                                               \
        (*VECOF(o)) OP floatof(o1)->f_value;            \
        USEo();

#define INT_VEC_BINOP(VEC, NEWVEC, VECOF, BINOP, OP)    \
    case ICI_TRI(TC_INT, VEC, BINOP):                   \
        if ((o = NEWVEC(VECOF(o1))) == nullptr)         \
        {                                               \
            FAIL();                                     \
        }                                               \
        (*VECOF(o)) OP intof(o0)->i_value;              \
        USEo();

#define FLOAT_VEC_BINOP(VEC, NEWVEC, VECOF, BINOP, OP)  \
    case ICI_TRI(TC_FLOAT, VEC, BINOP):                 \
        if ((o = NEWVEC(VECOF(o1))) == nullptr)         \
        {                                               \
            FAIL();                                     \
        }                                               \
        (*VECOF(o)) OP floatof(o0)->f_value;            \
        USEo();

#define VEC_OPS(VEC, NEWVEC, VECOF)                     \
                                                        \
    VEC_VEC_ASSIGNOP(VEC, VECOF, T_PLUSEQ, +=)          \
    VEC_VEC_ASSIGNOP(VEC, VECOF, T_MINUSEQ, -=)         \
    VEC_VEC_ASSIGNOP(VEC, VECOF, T_ASTERIXEQ, *=)       \
    VEC_VEC_ASSIGNOP(VEC, VECOF, T_SLASHEQ, /=)         \
                                                        \
    VEC_INT_ASSIGNOP(VEC, VECOF, T_PLUSEQ, +=)          \
    VEC_INT_ASSIGNOP(VEC, VECOF, T_MINUSEQ, -=)         \
    VEC_INT_ASSIGNOP(VEC, VECOF, T_ASTERIXEQ, *=)       \
    VEC_INT_ASSIGNOP(VEC, VECOF, T_SLASHEQ, /=)         \
                                                        \
    VEC_FLOAT_ASSIGNOP(VEC, VECOF, T_PLUSEQ, +=)        \
    VEC_FLOAT_ASSIGNOP(VEC, VECOF, T_MINUSEQ, -=)       \
    VEC_FLOAT_ASSIGNOP(VEC, VECOF, T_ASTERIXEQ, *=)     \
    VEC_FLOAT_ASSIGNOP(VEC, VECOF, T_SLASHEQ, /=)       \
                                                        \
    VEC_VEC_BINOP(VEC, VECOF, T_PLUS, +=)               \
    VEC_VEC_BINOP(VEC, VECOF, T_MINUS, -=)              \
    VEC_VEC_BINOP(VEC, VECOF, T_ASTERIX, *=)            \
    VEC_VEC_BINOP(VEC, VECOF, T_SLASH, /=)              \
                                                        \
    VEC_INT_BINOP(VEC, NEWVEC, VECOF, T_PLUS, +=)       \
    VEC_INT_BINOP(VEC, NEWVEC, VECOF, T_MINUS, -=)      \
    VEC_INT_BINOP(VEC, NEWVEC, VECOF, T_ASTERIX, *=)    \
    VEC_INT_BINOP(VEC, NEWVEC, VECOF, T_SLASH, /=)      \
                                                        \
    INT_VEC_BINOP(VEC, NEWVEC, VECOF, T_PLUS, +=)       \
    INT_VEC_BINOP(VEC, NEWVEC, VECOF, T_ASTERIX, *=)    \
                                                        \
    VEC_FLOAT_BINOP(VEC, NEWVEC, VECOF, T_PLUS, +=)     \
    VEC_FLOAT_BINOP(VEC, NEWVEC, VECOF, T_MINUS, -=)    \
    VEC_FLOAT_BINOP(VEC, NEWVEC, VECOF, T_ASTERIX, *=)  \
    VEC_FLOAT_BINOP(VEC, NEWVEC, VECOF, T_SLASH, /=)    \
                                                        \
    FLOAT_VEC_BINOP(VEC, NEWVEC, VECOF, T_PLUS, +=)     \
    FLOAT_VEC_BINOP(VEC, NEWVEC, VECOF, T_ASTERIX, *=)


VEC_OPS(TC_VEC32F, new_vec32f, vec32fof)
VEC_OPS(TC_VEC64F, new_vec64f, vec64fof)
vecmismatch:
    set_error("vec size mis-match: %lu vs. %lu", vec32fof(o0)->v_size, vec32fof(o1)->v_size);
    FAIL();
