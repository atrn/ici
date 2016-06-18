/*
 * This code is in an include file because some compilers may not handle
 * the large function and switch statement which happens in exec.c.
 * See ici_op_binop() in arith.c if you have this problem.
 *
 * This is where run-time binary operator "arithmetic" happens.
 */

#ifndef BINOPFUNC
// This uses knowledge of the exec switch in exec.c to avoid chains of
// gotos.  The continue_with_same_pc label is defined there.
//
#define USE0()					\
    do						\
    {						\
        ici_os.a_top[-2] = ici_objof(ici_zero);	\
	--ici_os.a_top;				\
	goto continue_with_same_pc;		\
    }						\
    while (0)

#define USE1()					\
    do						\
    {						\
	ici_os.a_top[-2] = ici_objof(ici_one);	\
	--ici_os.a_top;				\
	goto continue_with_same_pc;		\
    }						\
    while (0)

#define USEo()					\
    do						\
    {						\
	ici_os.a_top[-2] = o;			\
	--ici_os.a_top;				\
	goto continue_with_same_pc;		\
    }						\
    while (0)

#define LOOSEo()				\
    do						\
    {						\
	ici_decref(o);				\
	ici_os.a_top[-2] = o;			\
	--ici_os.a_top;				\
	goto continue_with_same_pc;		\
    }						\
    while (0)

#else
#define USE0() 	        goto use0
#define USE1() 	        goto use1
#define USEo() 	        goto useo
#define LOOSEo() 	goto looseo
#endif

#define FAIL()          goto fail
#define USEi(I)         i = (I); goto usei
#define USEf(F)         f = (F); goto usef
#define MISMATCH()      goto mismatch

/*
 * On entry, o = ici_xs.a_top[-1], technically, but ici_xs.a_top has been pre-decremented
 * so it isn't really there.
 */
{
    ici_obj_t  *o0;
    ici_obj_t  *o1;
    long       i;
    double              f;
    int                 can_temp;

#define SWAP()          (o = o0, o0 = o1, o1 = o)

    o0 = ici_os.a_top[-2];
    o1 = ici_os.a_top[-1];
    can_temp = ici_opof(o)->op_ecode == ICI_OP_BINOP_FOR_TEMP;
    if (o0->o_tcode > ICI_TC_MAX_BINOP || o1->o_tcode > ICI_TC_MAX_BINOP)
    {
        goto others;
    }
    switch (ICI_TRI(o0->o_tcode, o1->o_tcode, ici_opof(o)->op_code))
    {
    /*
     * Pure integer operations.
     */
    case ICI_TRI(ICI_TC_INT, ICI_TC_INT, T_ASTERIX):
    case ICI_TRI(ICI_TC_INT, ICI_TC_INT, T_ASTERIXEQ):
        USEi(ici_intof(o0)->i_value * ici_intof(o1)->i_value);

    case ICI_TRI(ICI_TC_INT, ICI_TC_INT, T_SLASH):
    case ICI_TRI(ICI_TC_INT, ICI_TC_INT, T_SLASHEQ):
        if (ici_intof(o1)->i_value == 0)
        {
            ici_error = "division by 0";
            FAIL();
        }
        USEi(ici_intof(o0)->i_value / ici_intof(o1)->i_value);

    case ICI_TRI(ICI_TC_INT, ICI_TC_INT, T_PERCENT):
    case ICI_TRI(ICI_TC_INT, ICI_TC_INT, T_PERCENTEQ):
        if (ici_intof(o1)->i_value == 0)
        {
            ici_error = "modulus by 0";
            FAIL();
        }
        USEi(ici_intof(o0)->i_value % ici_intof(o1)->i_value);

    case ICI_TRI(ICI_TC_INT, ICI_TC_INT, T_PLUS):
    case ICI_TRI(ICI_TC_INT, ICI_TC_INT, T_PLUSEQ):
        USEi(ici_intof(o0)->i_value + ici_intof(o1)->i_value);

    case ICI_TRI(ICI_TC_INT, ICI_TC_INT, T_MINUS):
    case ICI_TRI(ICI_TC_INT, ICI_TC_INT, T_MINUSEQ):
        USEi(ici_intof(o0)->i_value - ici_intof(o1)->i_value);

    case ICI_TRI(ICI_TC_INT, ICI_TC_INT, T_GRTGRT):
    case ICI_TRI(ICI_TC_INT, ICI_TC_INT, T_GRTGRTEQ):
        USEi(ici_intof(o0)->i_value >> ici_intof(o1)->i_value);

    case ICI_TRI(ICI_TC_INT, ICI_TC_INT, T_LESSLESS):
    case ICI_TRI(ICI_TC_INT, ICI_TC_INT, T_LESSLESSEQ):
        USEi(ici_intof(o0)->i_value << ici_intof(o1)->i_value);

    case ICI_TRI(ICI_TC_INT, ICI_TC_INT, T_LESS):
        if (ici_intof(o0)->i_value < ici_intof(o1)->i_value)
        {
            USE1();
        }
        USE0();

    case ICI_TRI(ICI_TC_INT, ICI_TC_INT, T_GRT):
        if (ici_intof(o0)->i_value > ici_intof(o1)->i_value)
        {
            USE1();
        }
        USE0();

    case ICI_TRI(ICI_TC_INT, ICI_TC_INT, T_LESSEQ):
        if (ici_intof(o0)->i_value <= ici_intof(o1)->i_value)
        {
            USE1();
        }
        USE0();

    case ICI_TRI(ICI_TC_INT, ICI_TC_INT, T_GRTEQ):
        if (ici_intof(o0)->i_value >= ici_intof(o1)->i_value)
        {
            USE1();
        }
        USE0();

    case ICI_TRI(ICI_TC_INT, ICI_TC_INT, T_EQEQ):
        if (ici_intof(o0)->i_value == ici_intof(o1)->i_value)
        {
            USE1();
        }
        USE0();

    case ICI_TRI(ICI_TC_INT, ICI_TC_INT, T_EXCLAMEQ):
        if (ici_intof(o0)->i_value != ici_intof(o1)->i_value)
        {
            USE1();
        }
        USE0();

    case ICI_TRI(ICI_TC_INT, ICI_TC_INT, T_AND):
    case ICI_TRI(ICI_TC_INT, ICI_TC_INT, T_ANDEQ):
        USEi(ici_intof(o0)->i_value & ici_intof(o1)->i_value);

    case ICI_TRI(ICI_TC_INT, ICI_TC_INT, T_CARET):
    case ICI_TRI(ICI_TC_INT, ICI_TC_INT, T_CARETEQ):
        USEi(ici_intof(o0)->i_value ^ ici_intof(o1)->i_value);

    case ICI_TRI(ICI_TC_INT, ICI_TC_INT, T_BAR):
    case ICI_TRI(ICI_TC_INT, ICI_TC_INT, T_BAREQ):
        USEi(ici_intof(o0)->i_value | ici_intof(o1)->i_value);

    /*
     * Pure floating point and mixed float, int operations...
     */
    case ICI_TRI(ICI_TC_FLOAT, ICI_TC_FLOAT, T_ASTERIX):
    case ICI_TRI(ICI_TC_FLOAT, ICI_TC_FLOAT, T_ASTERIXEQ):
        USEf(ici_floatof(o0)->f_value * ici_floatof(o1)->f_value);

    case ICI_TRI(ICI_TC_FLOAT, ICI_TC_INT, T_ASTERIX):
    case ICI_TRI(ICI_TC_FLOAT, ICI_TC_INT, T_ASTERIXEQ):
        USEf(ici_floatof(o0)->f_value * ici_intof(o1)->i_value);

    case ICI_TRI(ICI_TC_INT, ICI_TC_FLOAT, T_ASTERIX):
    case ICI_TRI(ICI_TC_INT, ICI_TC_FLOAT, T_ASTERIXEQ):
        USEf(ici_intof(o0)->i_value * ici_floatof(o1)->f_value);

    case ICI_TRI(ICI_TC_FLOAT, ICI_TC_FLOAT, T_SLASH):
    case ICI_TRI(ICI_TC_FLOAT, ICI_TC_FLOAT, T_SLASHEQ):
        if (ici_floatof(o1)->f_value == 0)
        {
            ici_error = "division by 0.0";
            FAIL();
        }
        USEf(ici_floatof(o0)->f_value / ici_floatof(o1)->f_value);

    case ICI_TRI(ICI_TC_FLOAT, ICI_TC_INT, T_SLASH):
    case ICI_TRI(ICI_TC_FLOAT, ICI_TC_INT, T_SLASHEQ):
        if (ici_intof(o1)->i_value == 0)
        {
            ici_error = "division by 0";
            FAIL();
        }
        USEf(ici_floatof(o0)->f_value / ici_intof(o1)->i_value);

    case ICI_TRI(ICI_TC_INT, ICI_TC_FLOAT, T_SLASH):
    case ICI_TRI(ICI_TC_INT, ICI_TC_FLOAT, T_SLASHEQ):
        if (ici_floatof(o1)->f_value == 0)
        {
            ici_error = "division by 0.0";
            FAIL();
        }
        USEf(ici_intof(o0)->i_value / ici_floatof(o1)->f_value);

    case ICI_TRI(ICI_TC_FLOAT, ICI_TC_FLOAT, T_PLUS):
    case ICI_TRI(ICI_TC_FLOAT, ICI_TC_FLOAT, T_PLUSEQ):
        USEf(ici_floatof(o0)->f_value + ici_floatof(o1)->f_value);

    case ICI_TRI(ICI_TC_FLOAT, ICI_TC_INT, T_PLUS):
    case ICI_TRI(ICI_TC_FLOAT, ICI_TC_INT, T_PLUSEQ):
        USEf(ici_floatof(o0)->f_value + ici_intof(o1)->i_value);

    case ICI_TRI(ICI_TC_INT, ICI_TC_FLOAT, T_PLUS):
    case ICI_TRI(ICI_TC_INT, ICI_TC_FLOAT, T_PLUSEQ):
        USEf(ici_intof(o0)->i_value + ici_floatof(o1)->f_value);

    case ICI_TRI(ICI_TC_FLOAT, ICI_TC_FLOAT, T_MINUS):
    case ICI_TRI(ICI_TC_FLOAT, ICI_TC_FLOAT, T_MINUSEQ):
        USEf(ici_floatof(o0)->f_value - ici_floatof(o1)->f_value);

    case ICI_TRI(ICI_TC_FLOAT, ICI_TC_INT, T_MINUS):
    case ICI_TRI(ICI_TC_FLOAT, ICI_TC_INT, T_MINUSEQ):
        USEf(ici_floatof(o0)->f_value - ici_intof(o1)->i_value);

    case ICI_TRI(ICI_TC_INT, ICI_TC_FLOAT, T_MINUS):
    case ICI_TRI(ICI_TC_INT, ICI_TC_FLOAT, T_MINUSEQ):
        USEf(ici_intof(o0)->i_value - ici_floatof(o1)->f_value);

    case ICI_TRI(ICI_TC_FLOAT, ICI_TC_FLOAT, T_LESS):
        if (ici_floatof(o0)->f_value < ici_floatof(o1)->f_value)
        {
            USE1();
        }
        USE0();

    case ICI_TRI(ICI_TC_FLOAT, ICI_TC_INT, T_LESS):
        if (ici_floatof(o0)->f_value < ici_intof(o1)->i_value)
        {
            USE1();
        }
        USE0();

    case ICI_TRI(ICI_TC_INT, ICI_TC_FLOAT, T_LESS):
        if (ici_intof(o0)->i_value < ici_floatof(o1)->f_value)
        {
            USE1();
        }
        USE0();

    case ICI_TRI(ICI_TC_FLOAT, ICI_TC_FLOAT, T_GRT):
        if (ici_floatof(o0)->f_value > ici_floatof(o1)->f_value)
        {
            USE1();
        }
        USE0();

    case ICI_TRI(ICI_TC_FLOAT, ICI_TC_INT, T_GRT):
        if (ici_floatof(o0)->f_value > ici_intof(o1)->i_value)
        {
            USE1();
        }
        USE0();

    case ICI_TRI(ICI_TC_INT, ICI_TC_FLOAT, T_GRT):
        if (ici_intof(o0)->i_value > ici_floatof(o1)->f_value)
        {
            USE1();
        }
        USE0();

    case ICI_TRI(ICI_TC_FLOAT, ICI_TC_FLOAT, T_LESSEQ):
        if (ici_floatof(o0)->f_value <= ici_floatof(o1)->f_value)
        {
            USE1();
        }
        USE0();

    case ICI_TRI(ICI_TC_FLOAT, ICI_TC_INT, T_LESSEQ):
        if (ici_floatof(o0)->f_value <= ici_intof(o1)->i_value)
        {
            USE1();
        }
        USE0();

    case ICI_TRI(ICI_TC_INT, ICI_TC_FLOAT, T_LESSEQ):
        if (ici_intof(o0)->i_value <= ici_floatof(o1)->f_value)
        {
            USE1();
        }
        USE0();

    case ICI_TRI(ICI_TC_FLOAT, ICI_TC_FLOAT, T_GRTEQ):
        if (ici_floatof(o0)->f_value >= ici_floatof(o1)->f_value)
        {
            USE1();
        }
        USE0();

    case ICI_TRI(ICI_TC_FLOAT, ICI_TC_INT, T_GRTEQ):
        if (ici_floatof(o0)->f_value >= ici_intof(o1)->i_value)
        {
            USE1();
        }
        USE0();

    case ICI_TRI(ICI_TC_INT, ICI_TC_FLOAT, T_GRTEQ):
        if (ici_intof(o0)->i_value >= ici_floatof(o1)->f_value)
        {
            USE1();
        }
        USE0();

    case ICI_TRI(ICI_TC_FLOAT, ICI_TC_FLOAT, T_EQEQ):
        if (ici_floatof(o0)->f_value == ici_floatof(o1)->f_value)
        {
            USE1();
        }
        USE0();

    case ICI_TRI(ICI_TC_FLOAT, ICI_TC_INT, T_EQEQ):
        if (ici_floatof(o0)->f_value == ici_intof(o1)->i_value)
        {
            USE1();
        }
        USE0();

    case ICI_TRI(ICI_TC_INT, ICI_TC_FLOAT, T_EQEQ):
        if (ici_intof(o0)->i_value == ici_floatof(o1)->f_value)
        {
            USE1();
        }
        USE0();

    case ICI_TRI(ICI_TC_FLOAT, ICI_TC_FLOAT, T_EXCLAMEQ):
        if (ici_floatof(o0)->f_value != ici_floatof(o1)->f_value)
        {
            USE1();
        }
        USE0();

    case ICI_TRI(ICI_TC_FLOAT, ICI_TC_INT, T_EXCLAMEQ):
        if (ici_floatof(o0)->f_value != ici_intof(o1)->i_value)
        {
            USE1();
        }
        USE0();

    case ICI_TRI(ICI_TC_INT, ICI_TC_FLOAT, T_EXCLAMEQ):
        if (ici_intof(o0)->i_value != ici_floatof(o1)->f_value)
        {
            USE1();
        }
        USE0();

    /*
     * Regular expression operators...
     */
    case ICI_TRI(ICI_TC_STRING, ICI_TC_REGEXP, T_TILDE):
        SWAP();
    case ICI_TRI(ICI_TC_REGEXP, ICI_TC_STRING, T_TILDE):
        if (ici_pcre_exec_simple(ici_regexpof(o0), ici_stringof(o1)) >= 0)
        {
            USE1();
        }
        USE0();

    case ICI_TRI(ICI_TC_STRING, ICI_TC_REGEXP, T_EXCLAMTILDE):
        SWAP();
    case ICI_TRI(ICI_TC_REGEXP, ICI_TC_STRING, T_EXCLAMTILDE):
        if (ici_pcre_exec_simple(ici_regexpof(o0), ici_stringof(o1)) < 0)
        {
            USE1();
        }
        USE0();

    case ICI_TRI(ICI_TC_STRING, ICI_TC_REGEXP, T_2TILDE):
    case ICI_TRI(ICI_TC_STRING, ICI_TC_REGEXP, T_2TILDEEQ):
        SWAP();
    case ICI_TRI(ICI_TC_REGEXP, ICI_TC_STRING, T_2TILDE):
    case ICI_TRI(ICI_TC_REGEXP, ICI_TC_STRING, T_2TILDEEQ):
        memset(ici_re_bra, 0, sizeof ici_re_bra);
        if (ici_pcre_exec_simple(ici_regexpof(o0), ici_stringof(o1)) < 0
            ||
            ici_re_bra[2] < 0 || ici_re_bra[3] < 0
        )
        {
            o = ici_null;
            USEo();
        }
        else
        {
            o = ici_objof
                (
                    ici_str_new
                    (
                        ici_stringof(o1)->s_chars + ici_re_bra[2],
                        ici_re_bra[3] - ici_re_bra[2]
                    )
                );
            if (o == NULL)
            {
                FAIL();
            }
        }
        LOOSEo();

    case ICI_TRI(ICI_TC_STRING, ICI_TC_REGEXP, T_3TILDE):
        SWAP();
    case ICI_TRI(ICI_TC_REGEXP, ICI_TC_STRING, T_3TILDE):
        memset(ici_re_bra, 0, sizeof ici_re_bra);
        ici_re_nbra = ici_pcre_exec_simple(ici_regexpof(o0), ici_stringof(o1));
        if (ici_re_nbra < 0)
        {
            o = ici_null;
            USEo();
        }
        if ((o = ici_objof(ici_array_new(ici_re_nbra))) == NULL)
        {
            FAIL();
        }
        for (i = 1; i < ici_re_nbra; ++i)
        {
            if (ici_re_bra[i*2] == -1)
            {
                if ((*ici_arrayof(o)->a_top = ici_objof(ici_str_alloc(0))) == NULL)
                {
                    FAIL();
                }
            }
            else if
            (
                (
                    *ici_arrayof(o)->a_top
                    =
                    ici_objof
                    (
                        ici_str_new
                        (
                            ici_stringof(o1)->s_chars + ici_re_bra[i*2],
                            ici_re_bra[(i * 2) + 1 ] - ici_re_bra[i * 2]
                        )
                    )
                )
                ==
                NULL
            )
            {
                FAIL();
            }
            ici_decref(*ici_arrayof(o)->a_top);
            ++ici_arrayof(o)->a_top;
        }
        LOOSEo();

    /*
     * Everything else...
     */
    case ICI_TRI(ICI_TC_PTR, ICI_TC_INT, T_MINUS):
    case ICI_TRI(ICI_TC_PTR, ICI_TC_INT, T_MINUSEQ):
        if (!ici_isint(ici_ptrof(o0)->p_key))
        {
            MISMATCH();
        }
        {
            ici_obj_t   *i;

            if ((i = ici_objof(ici_int_new(ici_intof(ici_ptrof(o0)->p_key)->i_value - ici_intof(o1)->i_value))) == NULL)
            {
                FAIL();
            }
            if ((o = ici_objof(ici_ptr_new(ici_ptrof(o0)->p_aggr, i))) == NULL)
            {
                FAIL();
            }
            ici_decref(i);
        }
        LOOSEo();
        
    case ICI_TRI(ICI_TC_INT, ICI_TC_PTR, T_PLUS):
    case ICI_TRI(ICI_TC_INT, ICI_TC_PTR, T_PLUSEQ):
        if (!ici_isint(ici_ptrof(o1)->p_key))
        {
            MISMATCH();
        }
        SWAP();
    case ICI_TRI(ICI_TC_PTR, ICI_TC_INT, T_PLUS):
    case ICI_TRI(ICI_TC_PTR, ICI_TC_INT, T_PLUSEQ):
        if (!ici_isint(ici_ptrof(o0)->p_key))
        {
            MISMATCH();
        }
        {
            ici_obj_t   *i;

            if ((i = ici_objof(ici_int_new(ici_intof(ici_ptrof(o0)->p_key)->i_value + ici_intof(o1)->i_value))) == NULL)
            {
                FAIL();
            }
            if ((o = ici_objof(ici_ptr_new(ici_ptrof(o0)->p_aggr, i))) == NULL)
            {
                FAIL();
            }
            ici_decref(i);
        }
        LOOSEo();

    case ICI_TRI(ICI_TC_STRING, ICI_TC_STRING, T_PLUS):
    case ICI_TRI(ICI_TC_STRING, ICI_TC_STRING, T_PLUSEQ):
        if ((o = ici_objof(ici_str_alloc(ici_stringof(o1)->s_nchars + ici_stringof(o0)->s_nchars))) == NULL)
        {
            FAIL();
        }
        memcpy
        (
            ici_stringof(o)->s_chars,
            ici_stringof(o0)->s_chars,
            ici_stringof(o0)->s_nchars
        );
        memcpy
        (
            ici_stringof(o)->s_chars + ici_stringof(o0)->s_nchars,
            ici_stringof(o1)->s_chars,
            ici_stringof(o1)->s_nchars + 1
        );
        o = ici_atom(o, 1);
        LOOSEo();

    case ICI_TRI(ICI_TC_ARRAY, ICI_TC_ARRAY, T_PLUS):
    case ICI_TRI(ICI_TC_ARRAY, ICI_TC_ARRAY, T_PLUSEQ):
        {
            ici_array_t *a;
            ptrdiff_t   z0;
            ptrdiff_t   z1;

            z0 = ici_array_nels(ici_arrayof(o0));
            z1 = ici_array_nels(ici_arrayof(o1));
            if ((a = ici_array_new(z0 + z1)) == NULL)
            {
                FAIL();
            }
            ici_array_gather(a->a_top, ici_arrayof(o0), 0, z0);
            a->a_top += z0;
            ici_array_gather(a->a_top, ici_arrayof(o1), 0, z1);
            a->a_top += z1;
            o = ici_objof(a);
        }
        LOOSEo();

    case ICI_TRI(ICI_TC_STRUCT, ICI_TC_STRUCT, T_PLUS):
    case ICI_TRI(ICI_TC_STRUCT, ICI_TC_STRUCT, T_PLUSEQ):
        {
            ici_struct_t   *s;
            ici_sslot_t *sl;
            int        i;

            if ((s = ici_structof(copy(o0))) == NULL)
            {
                FAIL();
            }
            sl = ici_structof(o1)->s_slots;
            for (i = 0; i < ici_structof(o1)->s_nslots; ++i, ++sl)
            {
                if (sl->sl_key == NULL)
                {
                    continue;
                }
                if (ici_assign(s, sl->sl_key, sl->sl_value))
                {
                    ici_decref(s);
                    FAIL();
                }
            }
            o = ici_objof(s);
        }
        LOOSEo();

    case ICI_TRI(ICI_TC_SET, ICI_TC_SET, T_PLUS):
    case ICI_TRI(ICI_TC_SET, ICI_TC_SET, T_PLUSEQ):
        {
            ici_set_t  *s;
            ici_obj_t  **sl;
            int        i;

            if ((s = ici_setof(copy(o0))) == NULL)
            {
                FAIL();
            }
            sl = ici_setof(o1)->s_slots;
            for (i = 0; i < ici_setof(o1)->s_nslots; ++i, ++sl)
            {
                if (*sl == NULL)
                {
                    continue;
                }
                if (ici_assign(s, *sl, ici_one))
                {
                    ici_decref(s);
                    FAIL();
                }
            }
            o = ici_objof(s);
        }
        LOOSEo();

    case ICI_TRI(ICI_TC_SET, ICI_TC_SET, T_MINUS):
    case ICI_TRI(ICI_TC_SET, ICI_TC_SET, T_MINUSEQ):
        {
            ici_set_t  *s;
            ici_obj_t  **sl;
            int        i;

            if ((s = ici_setof(copy(o0))) == NULL)
            {
                FAIL();
            }
            sl = ici_setof(o1)->s_slots;
            for (i = 0; i < ici_setof(o1)->s_nslots; ++i, ++sl)
            {
                if (*sl == NULL)
                {
                    continue;
                }
                if (ici_assign(s, *sl, ici_null))
                {
                    ici_decref(s);
                    FAIL();
                }
            }
            o = ici_objof(s);
        }
        LOOSEo();

    case ICI_TRI(ICI_TC_SET, ICI_TC_SET, T_ASTERIX):
    case ICI_TRI(ICI_TC_SET, ICI_TC_SET, T_ASTERIXEQ):
        {
            ici_set_t  *s;
            ici_obj_t  **sl;
            int        i;

            if (ici_setof(o0)->s_nels > ici_setof(o1)->s_nels)
            {
                SWAP();
            }
            if ((s = ici_set_new()) == NULL)
            {
                FAIL();
            }
            sl = ici_setof(o0)->s_slots;
            for (i = 0; i < ici_setof(o0)->s_nslots; ++i, ++sl)
            {
                if (*sl == NULL)
                {
                    continue;
                }
                if
                (
                    ici_fetch(o1, *sl) != ici_null
                    &&
                    ici_assign(s, *sl, ici_one)
                )
                {
                    ici_decref(s);
                    FAIL();
                }
            }
            o = ici_objof(s);
        }
        LOOSEo();

    case ICI_TRI(ICI_TC_SET, ICI_TC_SET, T_GRTEQ):
        o = ici_set_issubset(ici_setof(o1), ici_setof(o0)) ? ici_objof(ici_one) : ici_objof(ici_zero);
        USEo();

    case ICI_TRI(ICI_TC_SET, ICI_TC_SET, T_LESSEQ):
        o = ici_set_issubset(ici_setof(o0), ici_setof(o1)) ? ici_objof(ici_one) : ici_objof(ici_zero);
        USEo();

    case ICI_TRI(ICI_TC_SET, ICI_TC_SET, T_GRT):
        o = ici_set_ispropersubset(ici_setof(o1), ici_setof(o0)) ? ici_objof(ici_one) : ici_objof(ici_zero);
        USEo();

    case ICI_TRI(ICI_TC_SET, ICI_TC_SET, T_LESS):
        o = ici_set_ispropersubset(ici_setof(o0), ici_setof(o1)) ? ici_objof(ici_one) : ici_objof(ici_zero);
        USEo();

    case ICI_TRI(ICI_TC_PTR, ICI_TC_PTR, T_MINUS):
    case ICI_TRI(ICI_TC_PTR, ICI_TC_PTR, T_MINUSEQ):
        if (!ici_isint(ici_ptrof(o1)->p_key) || !ici_isint(ici_ptrof(o0)->p_key))
        {
            MISMATCH();
        }
        if ((o = ici_objof(ici_int_new(ici_intof(ici_ptrof(o0)->p_key)->i_value - ici_intof(ici_ptrof(o1)->p_key)->i_value))) == NULL)
        {
              FAIL();
        }
        LOOSEo();

    case ICI_TRI(ICI_TC_STRING, ICI_TC_STRING, T_LESS):
    case ICI_TRI(ICI_TC_STRING, ICI_TC_STRING, T_GRT):
    case ICI_TRI(ICI_TC_STRING, ICI_TC_STRING, T_LESSEQ):
    case ICI_TRI(ICI_TC_STRING, ICI_TC_STRING, T_GRTEQ):
        {
            int         compare;
            ici_str_t   *s1;
            ici_str_t   *s2;

            s1 = ici_stringof(o0);
            s2 = ici_stringof(o1);
            compare = s1->s_nchars < s2->s_nchars ? s1->s_nchars : s2->s_nchars;
            compare = memcmp(s1->s_chars, s2->s_chars, compare);
            if (compare == 0)
            {
                if (s1->s_nchars < s2->s_nchars)
                {
                    compare = -1;
                }
                else if (s1->s_nchars > s2->s_nchars)
                {
                    compare = 1;
                }
            }
            switch (ici_opof(o)->op_code)
            {
            case t_subtype(T_LESS):   if (compare < 0) USE1(); break;
            case t_subtype(T_GRT):    if (compare > 0) USE1(); break;
            case t_subtype(T_LESSEQ): if (compare <= 0) USE1(); break;
            case t_subtype(T_GRTEQ):  if (compare >= 0) USE1(); break;
            }
        }
        USE0();

    case ICI_TRI(ICI_TC_PTR, ICI_TC_PTR, T_LESS):
        if (!ici_isint(ici_ptrof(o1)->p_key) || !ici_isint(ici_ptrof(o0)->p_key))
        {
            MISMATCH();
        }
        if (ici_intof(ici_ptrof(o0)->p_key)->i_value < ici_intof(ici_ptrof(o1)->p_key)->i_value)
        {
            USE1();
        }
        USE0();

    case ICI_TRI(ICI_TC_PTR, ICI_TC_PTR, T_GRTEQ):
        if (!ici_isint(ici_ptrof(o1)->p_key) || !ici_isint(ici_ptrof(o0)->p_key))
        {
            MISMATCH();
        }
        if (ici_intof(ici_ptrof(o0)->p_key)->i_value >=ici_intof(ici_ptrof(o1)->p_key)->i_value)
        {
            USE1();
        }
        USE0();

    case ICI_TRI(ICI_TC_PTR, ICI_TC_PTR, T_LESSEQ):
        if (!ici_isint(ici_ptrof(o1)->p_key) || !ici_isint(ici_ptrof(o0)->p_key))
        {
            MISMATCH();
        }
        if (ici_intof(ici_ptrof(o0)->p_key)->i_value <=ici_intof(ici_ptrof(o1)->p_key)->i_value)
        {
            USE1();
        }
        USE0();

    case ICI_TRI(ICI_TC_PTR, ICI_TC_PTR, T_GRT):
        if (!ici_isint(ici_ptrof(o1)->p_key) || !ici_isint(ici_ptrof(o0)->p_key))
        {
            MISMATCH();
        }
        if (ici_intof(ici_ptrof(o0)->p_key)->i_value > ici_intof(ici_ptrof(o1)->p_key)->i_value)
        {
            USE1();
        }
        USE0();

    default:
    others:
        switch (ici_opof(o)->op_code)
        {
        case t_subtype(T_PLUSEQ):
	    if (o0->o_tcode == ICI_TC_SET)
	    {
		if (ici_assign(o0, o1, ici_one))
                {
		    FAIL();
                }
		o = o0;
		USEo();
	    }
	    MISMATCH();

	case t_subtype(T_MINUSEQ):
	    if (o0->o_tcode == ICI_TC_SET)
	    {
		if (ici_set_unassign(ici_setof(o0), o1))
                {
		    FAIL();
                }
		o = o0;
		USEo();
	    }
	    MISMATCH();

        case t_subtype(T_EQEQ):
            if (ici_typeof(o0) == ici_typeof(o1) && cmp(o0, o1) == 0)
            {
                USE1();
            }
            USE0();

        case t_subtype(T_EXCLAMEQ):
            if (!(ici_typeof(o0) == ici_typeof(o1) && cmp(o0, o1) == 0))
            {
                USE1();
            }
            USE0();
        }
        /*FALLTHROUGH*/
    mismatch:
        {
            char        n1[30];
            char        n2[30];

            sprintf(buf, "attempt to perform \"%s %s %s\"",
                ici_objname(n1, o0),
                ici_binop_name(ici_opof(o)->op_code),
                ici_objname(n2, o1));
        }
        ici_error = buf;
        FAIL();
    }

#ifdef BINOPFUNC
use0:
    ici_os.a_top[-2] = ici_objof(ici_zero);
    goto done;

use1:
    ici_os.a_top[-2] = ici_objof(ici_one);
    goto done;
#endif

usef:
    if (can_temp)
    {
        int             n;

        n = &ici_os.a_top[-2] - ici_os.a_base;
        if (ici_stk_probe(ici_exec->x_os_temp_cache, n))
        {
            FAIL();
        }
        if ((o = ici_exec->x_os_temp_cache->a_base[n]) == ici_null)
        {
            if ((o = ici_objof(ici_talloc(ici_ostemp_t))) == NULL)
            {
                FAIL();
            }
            ici_exec->x_os_temp_cache->a_base[n] = o;
            ici_rego(o);
        }
        ICI_OBJ_SET_TFNZ(o, ICI_TC_FLOAT, ICI_O_TEMP, 0, sizeof(ici_ostemp_t));
        ici_floatof(o)->f_value = f;
        USEo();
    }
    /*
     * The following in-line expansion of float creation replaces, and
     * this should be equivalent to, this old code:
     *
     * if ((o = ici_objof(ici_float_new(f))) == NULL)
     *     FAIL();
     * LOOSEo();
     */

    /*
     * In-line expansion of float creation.
     */
    {
        ici_obj_t               **po;
        union
        {
            double              f;
            int32_t             l[2];
        }
            v;
        unsigned long           h;

        /*
         * If this assert fails, we should switch back to a an explicit call
         * to hash_float().
         */
        assert(sizeof ici_floatof(o)->f_value == 2 * sizeof(unsigned long));
        v.f = f;
        h = FLOAT_PRIME + v.l[0] + v.l[1] * 31;
        h ^= (h >> 12) ^ (h >> 24);
        for
        (
            po = &ici_atoms[ici_atom_hash_index(h)];
            (o = *po) != NULL;
            --po < ici_atoms ? po = ici_atoms + ici_atomsz - 1 : NULL
        )
        {
            if (ici_isfloat(o) && DBL_BIT_CMP(&ici_floatof(o)->f_value, &v.f))
            {
                USEo();
            }
        }
        ++ici_supress_collect;
        if ((o = ici_objof(ici_talloc(ici_float_t))) == NULL)
        {
            --ici_supress_collect;
            FAIL();
        }
        ICI_OBJ_SET_TFNZ(o, ICI_TC_FLOAT, ICI_O_ATOM, 1, sizeof(ici_float_t));
        ici_floatof(o)->f_value = f;
        ici_rego(o);
        assert(h == hash(o));
        --ici_supress_collect;
        ICI_STORE_ATOM_AND_COUNT(po, o);
        LOOSEo();
    }

usei:
    if (can_temp)
    {
        int             n;

        n = &ici_os.a_top[-2] - ici_os.a_base;
        if (UNLIKELY(ici_stk_probe(ici_exec->x_os_temp_cache, n)))
        {
            FAIL();
        }
        if ((o = ici_exec->x_os_temp_cache->a_base[n]) == ici_null)
        {
            if ((o = ici_objof(ici_talloc(ici_ostemp_t))) == NULL)
            {
                FAIL();
            }
            ici_exec->x_os_temp_cache->a_base[n] = o;
            ici_rego(o);
        }
        ICI_OBJ_SET_TFNZ(o, ICI_TC_INT, ICI_O_TEMP, 0, sizeof(ici_ostemp_t));
        ici_intof(o)->i_value = i;
        USEo();
    }
    /*
     * In-line expansion of atom_int() from object.c. Following that, and
     * merged with it, is in-line atom creation.
     */
    if ((i & ~ICI_SMALL_INT_MASK) == 0)
    {
        o = ici_objof(ici_small_ints[i]);
        USEo();
    }
    {
        ici_obj_t      **po;

        for
        (
            po = &ici_atoms[ici_atom_hash_index((unsigned long)i * INT_PRIME)];
            (o = *po) != NULL;
            --po < ici_atoms ? po = ici_atoms + ici_atomsz - 1 : NULL
        )
        {
            if (ici_isint(o) && ici_intof(o)->i_value == i)
            {
                USEo();
            }
        }
        ++ici_supress_collect;
        if ((o = ici_objof(ici_talloc(ici_int_t))) == NULL)
        {
            --ici_supress_collect;
            FAIL();
        }
        ICI_OBJ_SET_TFNZ(o, ICI_TC_INT, ICI_O_ATOM, 1, sizeof(ici_int_t));
        ici_intof(o)->i_value = i;
        ici_rego(o);
        --ici_supress_collect;
        ICI_STORE_ATOM_AND_COUNT(po, o);
    }

#ifdef BINOPFUNC
looseo:
    ici_decref(o);
useo:
    ici_os.a_top[-2] = o;
done:
#else // non-binop func version does not 'goto' the labels above
    ici_decref(o);
    ici_os.a_top[-2] = o;
#endif
    --ici_os.a_top;
    /*--ici_xs.a_top; Don't do this because it has been pre-done. */
}
