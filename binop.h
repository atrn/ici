// -*- mode:c++ -*-

/*
 * This code is in an include file because some compilers may not handle
 * the large function and switch statement which happens in exec.c.
 * See op_binop() in arith.c if you have this problem.
 *
 * This is where run-time binary operator "arithmetic" happens.
 */

#ifndef BINOPFUNC

#define ICI_TRI(a,b,t)      (((((a) << 4) + b) << 6) + t_subtype(t))

// This uses knowledge of the exec switch in exec.c to avoid chains of
// gotos.  The continue_with_same_pc label is defined there.
//
#define USE0()					\
    do						\
    {						\
        os.a_top[-2] = o_zero;                  \
	--os.a_top;				\
	goto continue_with_same_pc;		\
    }						\
    while (0)

#define USE1()					\
    do						\
    {						\
	os.a_top[-2] = o_one;                   \
	--os.a_top;				\
	goto continue_with_same_pc;		\
    }						\
    while (0)

#define USEo()					\
    do						\
    {						\
	os.a_top[-2] = o;			\
	--os.a_top;				\
	goto continue_with_same_pc;		\
    }						\
    while (0)

#define LOOSEo()				\
    do						\
    {						\
	decref(o);				\
	os.a_top[-2] = o;			\
	--os.a_top;				\
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
#define VECMISMATCH()   goto vecmismatch

#define MATCHVEC(VECOF)                                   \
    if (VECOF(o0)->v_capacity != VECOF(o1)->v_capacity)   \
    {                                                     \
        VECMISMATCH();                                    \
    }

/*
 * On entry, o = xs.a_top[-1], technically, but xs.a_top has been pre-decremented
 * so it isn't really there.
 */
{
    object *o0;
    object *o1;
    long    i;
    double  f;
    bool    can_temp;

#define SWAP()          (o = o0, o0 = o1, o1 = o)

    o0 = os.a_top[-2];
    o1 = os.a_top[-1];
    can_temp = opof(o)->op_ecode == OP_BINOP_FOR_TEMP;
    // if (o0->o_tcode > TC_MAX_BINOP || o1->o_tcode > TC_MAX_BINOP) {
    //     goto others;
    // }
    switch (ICI_TRI(o0->o_tcode, o1->o_tcode, opof(o)->op_code)) {
    /*
     * Pure integer operations.
     */
    case ICI_TRI(TC_INT, TC_INT, T_ASTERIX):
    case ICI_TRI(TC_INT, TC_INT, T_ASTERIXEQ):
        USEi(intof(o0)->i_value * intof(o1)->i_value);

    case ICI_TRI(TC_INT, TC_INT, T_SLASH):
    case ICI_TRI(TC_INT, TC_INT, T_SLASHEQ):
        if (intof(o1)->i_value == 0) {
            set_error("division by 0");
            FAIL();
        }
        USEi(intof(o0)->i_value / intof(o1)->i_value);

    case ICI_TRI(TC_INT, TC_INT, T_PERCENT):
    case ICI_TRI(TC_INT, TC_INT, T_PERCENTEQ):
        if (intof(o1)->i_value == 0) {
            set_error("modulus by 0");
            FAIL();
        }
        USEi(intof(o0)->i_value % intof(o1)->i_value);

    case ICI_TRI(TC_INT, TC_INT, T_PLUS):
    case ICI_TRI(TC_INT, TC_INT, T_PLUSEQ):
        USEi(intof(o0)->i_value + intof(o1)->i_value);

    case ICI_TRI(TC_INT, TC_INT, T_MINUS):
    case ICI_TRI(TC_INT, TC_INT, T_MINUSEQ):
        USEi(intof(o0)->i_value - intof(o1)->i_value);

    case ICI_TRI(TC_INT, TC_INT, T_GRTGRT):
    case ICI_TRI(TC_INT, TC_INT, T_GRTGRTEQ):
        USEi(intof(o0)->i_value >> intof(o1)->i_value);

    case ICI_TRI(TC_INT, TC_INT, T_LESSLESS):
    case ICI_TRI(TC_INT, TC_INT, T_LESSLESSEQ):
        USEi(intof(o0)->i_value << intof(o1)->i_value);

    case ICI_TRI(TC_INT, TC_INT, T_LESS):
        if (intof(o0)->i_value < intof(o1)->i_value) {
            USE1();
        }
        USE0();

    case ICI_TRI(TC_INT, TC_INT, T_GRT):
        if (intof(o0)->i_value > intof(o1)->i_value) {
            USE1();
        }
        USE0();

    case ICI_TRI(TC_INT, TC_INT, T_LESSEQ):
        if (intof(o0)->i_value <= intof(o1)->i_value) {
            USE1();
        }
        USE0();

    case ICI_TRI(TC_INT, TC_INT, T_GRTEQ):
        if (intof(o0)->i_value >= intof(o1)->i_value) {
            USE1();
        }
        USE0();

    case ICI_TRI(TC_INT, TC_INT, T_EQEQ):
        if (intof(o0)->i_value == intof(o1)->i_value) {
            USE1();
        }
        USE0();

    case ICI_TRI(TC_INT, TC_INT, T_EXCLAMEQ):
        if (intof(o0)->i_value != intof(o1)->i_value) {
            USE1();
        }
        USE0();

    case ICI_TRI(TC_INT, TC_INT, T_AND):
    case ICI_TRI(TC_INT, TC_INT, T_ANDEQ):
        USEi(intof(o0)->i_value & intof(o1)->i_value);

    case ICI_TRI(TC_INT, TC_INT, T_CARET):
    case ICI_TRI(TC_INT, TC_INT, T_CARETEQ):
        USEi(intof(o0)->i_value ^ intof(o1)->i_value);

    case ICI_TRI(TC_INT, TC_INT, T_BAR):
    case ICI_TRI(TC_INT, TC_INT, T_BAREQ):
        USEi(intof(o0)->i_value | intof(o1)->i_value);

    /*
     * Pure floating point and mixed float, int operations...
     */
    case ICI_TRI(TC_FLOAT, TC_FLOAT, T_ASTERIX):
    case ICI_TRI(TC_FLOAT, TC_FLOAT, T_ASTERIXEQ):
        USEf(floatof(o0)->f_value * floatof(o1)->f_value);

    case ICI_TRI(TC_FLOAT, TC_INT, T_ASTERIX):
    case ICI_TRI(TC_FLOAT, TC_INT, T_ASTERIXEQ):
        USEf(floatof(o0)->f_value * intof(o1)->i_value);

    case ICI_TRI(TC_INT, TC_FLOAT, T_ASTERIX):
    case ICI_TRI(TC_INT, TC_FLOAT, T_ASTERIXEQ):
        USEf(intof(o0)->i_value * floatof(o1)->f_value);

    case ICI_TRI(TC_FLOAT, TC_FLOAT, T_SLASH):
    case ICI_TRI(TC_FLOAT, TC_FLOAT, T_SLASHEQ):
        if (floatof(o1)->f_value == 0) {
            set_error("division by 0.0");
            FAIL();
        }
        USEf(floatof(o0)->f_value / floatof(o1)->f_value);

    case ICI_TRI(TC_FLOAT, TC_INT, T_SLASH):
    case ICI_TRI(TC_FLOAT, TC_INT, T_SLASHEQ):
        if (intof(o1)->i_value == 0) {
            set_error("division by 0");
            FAIL();
        }
        USEf(floatof(o0)->f_value / intof(o1)->i_value);

    case ICI_TRI(TC_INT, TC_FLOAT, T_SLASH):
    case ICI_TRI(TC_INT, TC_FLOAT, T_SLASHEQ):
        if (floatof(o1)->f_value == 0) {
            set_error("division by 0.0");
            FAIL();
        }
        USEf(intof(o0)->i_value / floatof(o1)->f_value);

    case ICI_TRI(TC_FLOAT, TC_FLOAT, T_PLUS):
    case ICI_TRI(TC_FLOAT, TC_FLOAT, T_PLUSEQ):
        USEf(floatof(o0)->f_value + floatof(o1)->f_value);

    case ICI_TRI(TC_FLOAT, TC_INT, T_PLUS):
    case ICI_TRI(TC_FLOAT, TC_INT, T_PLUSEQ):
        USEf(floatof(o0)->f_value + intof(o1)->i_value);

    case ICI_TRI(TC_INT, TC_FLOAT, T_PLUS):
    case ICI_TRI(TC_INT, TC_FLOAT, T_PLUSEQ):
        USEf(intof(o0)->i_value + floatof(o1)->f_value);

    case ICI_TRI(TC_FLOAT, TC_FLOAT, T_MINUS):
    case ICI_TRI(TC_FLOAT, TC_FLOAT, T_MINUSEQ):
        USEf(floatof(o0)->f_value - floatof(o1)->f_value);

    case ICI_TRI(TC_FLOAT, TC_INT, T_MINUS):
    case ICI_TRI(TC_FLOAT, TC_INT, T_MINUSEQ):
        USEf(floatof(o0)->f_value - intof(o1)->i_value);

    case ICI_TRI(TC_INT, TC_FLOAT, T_MINUS):
    case ICI_TRI(TC_INT, TC_FLOAT, T_MINUSEQ):
        USEf(intof(o0)->i_value - floatof(o1)->f_value);

    case ICI_TRI(TC_FLOAT, TC_FLOAT, T_LESS):
        if (floatof(o0)->f_value < floatof(o1)->f_value) {
            USE1();
        }
        USE0();

    case ICI_TRI(TC_FLOAT, TC_INT, T_LESS):
        if (floatof(o0)->f_value < intof(o1)->i_value) {
            USE1();
        }
        USE0();

    case ICI_TRI(TC_INT, TC_FLOAT, T_LESS):
        if (intof(o0)->i_value < floatof(o1)->f_value) {
            USE1();
        }
        USE0();

    case ICI_TRI(TC_FLOAT, TC_FLOAT, T_GRT):
        if (floatof(o0)->f_value > floatof(o1)->f_value) {
            USE1();
        }
        USE0();

    case ICI_TRI(TC_FLOAT, TC_INT, T_GRT):
        if (floatof(o0)->f_value > intof(o1)->i_value) {
            USE1();
        }
        USE0();

    case ICI_TRI(TC_INT, TC_FLOAT, T_GRT):
        if (intof(o0)->i_value > floatof(o1)->f_value) {
            USE1();
        }
        USE0();

    case ICI_TRI(TC_FLOAT, TC_FLOAT, T_LESSEQ):
        if (floatof(o0)->f_value <= floatof(o1)->f_value) {
            USE1();
        }
        USE0();

    case ICI_TRI(TC_FLOAT, TC_INT, T_LESSEQ):
        if (floatof(o0)->f_value <= intof(o1)->i_value) {
            USE1();
        }
        USE0();

    case ICI_TRI(TC_INT, TC_FLOAT, T_LESSEQ):
        if (intof(o0)->i_value <= floatof(o1)->f_value) {
            USE1();
        }
        USE0();

    case ICI_TRI(TC_FLOAT, TC_FLOAT, T_GRTEQ):
        if (floatof(o0)->f_value >= floatof(o1)->f_value) {
            USE1();
        }
        USE0();

    case ICI_TRI(TC_FLOAT, TC_INT, T_GRTEQ):
        if (floatof(o0)->f_value >= intof(o1)->i_value) {
            USE1();
        }
        USE0();

    case ICI_TRI(TC_INT, TC_FLOAT, T_GRTEQ):
        if (intof(o0)->i_value >= floatof(o1)->f_value) {
            USE1();
        }
        USE0();

    case ICI_TRI(TC_FLOAT, TC_FLOAT, T_EQEQ):
        if (floatof(o0)->f_value == floatof(o1)->f_value) {
            USE1();
        }
        USE0();

    case ICI_TRI(TC_FLOAT, TC_INT, T_EQEQ):
        if (floatof(o0)->f_value == intof(o1)->i_value) {
            USE1();
        }
        USE0();

    case ICI_TRI(TC_INT, TC_FLOAT, T_EQEQ):
        if (intof(o0)->i_value == floatof(o1)->f_value) {
            USE1();
        }
        USE0();

    case ICI_TRI(TC_FLOAT, TC_FLOAT, T_EXCLAMEQ):
        if (floatof(o0)->f_value != floatof(o1)->f_value) {
            USE1();
        }
        USE0();

    case ICI_TRI(TC_FLOAT, TC_INT, T_EXCLAMEQ):
        if (floatof(o0)->f_value != intof(o1)->i_value) {
            USE1();
        }
        USE0();

    case ICI_TRI(TC_INT, TC_FLOAT, T_EXCLAMEQ):
        if (intof(o0)->i_value != floatof(o1)->f_value) {
            USE1();
        }
        USE0();

    /*
     * Regular expression operators...
     */
    case ICI_TRI(TC_STRING, TC_REGEXP, T_TILDE):
        SWAP();
        /*FALLTHROUGH*/
    case ICI_TRI(TC_REGEXP, TC_STRING, T_TILDE):
        if (ici_pcre_exec_simple(regexpof(o0), stringof(o1)) >= 0) {
            USE1();
        }
        USE0();

    case ICI_TRI(TC_STRING, TC_REGEXP, T_EXCLAMTILDE):
        SWAP();
        /*FALLTHROUGH*/
    case ICI_TRI(TC_REGEXP, TC_STRING, T_EXCLAMTILDE):
        if (ici_pcre_exec_simple(regexpof(o0), stringof(o1)) < 0) {
            USE1();
        }
        USE0();

    case ICI_TRI(TC_STRING, TC_REGEXP, T_2TILDE):
    case ICI_TRI(TC_STRING, TC_REGEXP, T_2TILDEEQ):
        SWAP();
        /*FALLTHROUGH*/
    case ICI_TRI(TC_REGEXP, TC_STRING, T_2TILDE):
    case ICI_TRI(TC_REGEXP, TC_STRING, T_2TILDEEQ):
        memset(re_bra, 0, sizeof re_bra);
        if (ici_pcre_exec_simple(regexpof(o0), stringof(o1)) < 0
            ||
            re_bra[2] < 0 || re_bra[3] < 0
        ) {
            o = null;
            USEo();
        } else {
            o = new_str
            (
                stringof(o1)->s_chars + re_bra[2],
                re_bra[3] - re_bra[2]
            );
            if (o == nullptr) {
                FAIL();
            }
        }
        LOOSEo();

    case ICI_TRI(TC_STRING, TC_REGEXP, T_3TILDE):
        SWAP();
        /*FALLTHROUGH*/
    case ICI_TRI(TC_REGEXP, TC_STRING, T_3TILDE):
        memset(re_bra, 0, sizeof re_bra);
        re_nbra = ici_pcre_exec_simple(regexpof(o0), stringof(o1));
        if (re_nbra < 0) {
            o = null;
            USEo();
        }
        if ((o = new_array(re_nbra)) == nullptr) {
            FAIL();
        }
        for (i = 1; i < re_nbra; ++i) {
            if (re_bra[i*2] == -1) {
                if ((*arrayof(o)->a_top = str_alloc(0)) == nullptr) {
                    FAIL();
                }
            }
            else if
            (
                (
                    *arrayof(o)->a_top
                    =
                    new_str
                    (
                        stringof(o1)->s_chars + re_bra[i*2],
                        re_bra[(i * 2) + 1 ] - re_bra[i * 2]
                    )
                )
                ==
                nullptr
            ) {
                FAIL();
            }
            decref(*arrayof(o)->a_top);
            ++arrayof(o)->a_top;
        }
        LOOSEo();

    /*
     * Everything else...
     */
    case ICI_TRI(TC_PTR, TC_INT, T_MINUS):
    case ICI_TRI(TC_PTR, TC_INT, T_MINUSEQ):
        if (!isint(ptrof(o0)->p_key)) {
            MISMATCH();
        }
        {
            object   *i;

            if ((i = new_int(intof(ptrof(o0)->p_key)->i_value - intof(o1)->i_value)) == nullptr) {
                FAIL();
            }
            if ((o = new_ptr(ptrof(o0)->p_aggr, i)) == nullptr) {
                FAIL();
            }
            decref(i);
        }
        LOOSEo();
        
    case ICI_TRI(TC_INT, TC_PTR, T_PLUS):
    case ICI_TRI(TC_INT, TC_PTR, T_PLUSEQ):
        if (!isint(ptrof(o1)->p_key)) {
            MISMATCH();
        }
        SWAP();
        /*FALLTHROUGH*/
    case ICI_TRI(TC_PTR, TC_INT, T_PLUS):
    case ICI_TRI(TC_PTR, TC_INT, T_PLUSEQ):
        if (!isint(ptrof(o0)->p_key)) {
            MISMATCH();
        }
        {
            object   *i;

            if ((i = new_int(intof(ptrof(o0)->p_key)->i_value + intof(o1)->i_value)) == nullptr) {
                FAIL();
            }
            if ((o = new_ptr(ptrof(o0)->p_aggr, i)) == nullptr) {
                FAIL();
            }
            decref(i);
        }
        LOOSEo();

    case ICI_TRI(TC_STRING, TC_STRING, T_PLUS):
    case ICI_TRI(TC_STRING, TC_STRING, T_PLUSEQ):
        if ((o = str_alloc(stringof(o1)->s_nchars + stringof(o0)->s_nchars)) == nullptr) {
            FAIL();
        }
        memcpy(stringof(o)->s_chars, stringof(o0)->s_chars, stringof(o0)->s_nchars);
        memcpy(stringof(o)->s_chars + stringof(o0)->s_nchars, stringof(o1)->s_chars, stringof(o1)->s_nchars + 1);
        o = atom(o, 1);
        LOOSEo();

    case ICI_TRI(TC_ARRAY, TC_ARRAY, T_PLUS):
    case ICI_TRI(TC_ARRAY, TC_ARRAY, T_PLUSEQ):
        {
            array *a;
            ptrdiff_t   z0;
            ptrdiff_t   z1;

            z0 = arrayof(o0)->len();
            z1 = arrayof(o1)->len();
            if ((a = new_array(z0 + z1)) == nullptr) {
                FAIL();
            }
            arrayof(o0)->gather(a->a_top, 0, z0);
            a->a_top += z0;
            arrayof(o1)->gather(a->a_top, 0, z1);
            a->a_top += z1;
            o = a;
        }
        LOOSEo();

    case ICI_TRI(TC_MAP, TC_MAP, T_PLUS):
    case ICI_TRI(TC_MAP, TC_MAP, T_PLUSEQ):
        {
            map    *s;
            slot  *sl;
            size_t  i;

            if ((s = mapof(copyof(o0))) == nullptr) {
                FAIL();
            }
            sl = mapof(o1)->s_slots;
            for (i = 0; i < mapof(o1)->s_nslots; ++i, ++sl) {
                if (sl->sl_key == nullptr) {
                    continue;
                }
                if (ici_assign(s, sl->sl_key, sl->sl_value)) {
                    decref(s);
                    FAIL();
                }
            }
            o = s;
        }
        LOOSEo();

    case ICI_TRI(TC_SET, TC_SET, T_PLUS):
    case ICI_TRI(TC_SET, TC_SET, T_PLUSEQ):
        {
            set     *s;
            object  **sl;
            size_t  i;

            if ((s = setof(copyof(o0))) == nullptr) {
                FAIL();
            }
            sl = setof(o1)->s_slots;
            for (i = 0; i < setof(o1)->s_nslots; ++i, ++sl) {
                if (*sl == nullptr){
                    continue;
                }
                if (ici_assign(s, *sl, o_one)) {
                    decref(s);
                    FAIL();
                }
            }
            o = s;
        }
        LOOSEo();

    case ICI_TRI(TC_SET, TC_SET, T_MINUS):
    case ICI_TRI(TC_SET, TC_SET, T_MINUSEQ):
        {
            set     *s;
            object  **sl;
            size_t  i;

            if ((s = setof(copyof(o0))) == nullptr) {
                FAIL();
            }
            sl = setof(o1)->s_slots;
            for (i = 0; i < setof(o1)->s_nslots; ++i, ++sl) {
                if (*sl == nullptr) {
                    continue;
                }
                if (ici_assign(s, *sl, null)) {
                    decref(s);
                    FAIL();
                }
            }
            o = s;
        }
        LOOSEo();

    case ICI_TRI(TC_SET, TC_SET, T_ASTERIX):
    case ICI_TRI(TC_SET, TC_SET, T_ASTERIXEQ):
        {
            set     *s;
            object  **sl;
            size_t  i;

            if (setof(o0)->s_nels > setof(o1)->s_nels) {
                SWAP();
            }
            if ((s = new_set()) == nullptr) {
                FAIL();
            }
            sl = setof(o0)->s_slots;
            for (i = 0; i < setof(o0)->s_nslots; ++i, ++sl) {
                if (*sl == nullptr) {
                    continue;
                }
                if (ici_fetch(o1, *sl) != null && ici_assign(s, *sl, o_one)) {
                    decref(s);
                    FAIL();
                }
            }
            o = s;
        }
        LOOSEo();

    case ICI_TRI(TC_SET, TC_SET, T_GRTEQ):
        o = set_issubset(setof(o1), setof(o0)) ? o_one : o_zero;
        USEo();

    case ICI_TRI(TC_SET, TC_SET, T_LESSEQ):
        o = set_issubset(setof(o0), setof(o1)) ? o_one : o_zero;
        USEo();

    case ICI_TRI(TC_SET, TC_SET, T_GRT):
        o = set_ispropersubset(setof(o1), setof(o0)) ? o_one : o_zero;
        USEo();

    case ICI_TRI(TC_SET, TC_SET, T_LESS):
        o = set_ispropersubset(setof(o0), setof(o1)) ? o_one : o_zero;
        USEo();

    case ICI_TRI(TC_PTR, TC_PTR, T_MINUS):
    case ICI_TRI(TC_PTR, TC_PTR, T_MINUSEQ):
        if (!isint(ptrof(o1)->p_key) || !isint(ptrof(o0)->p_key)) {
            MISMATCH();
        }
        if ((o = new_int(intof(ptrof(o0)->p_key)->i_value - intof(ptrof(o1)->p_key)->i_value)) == nullptr) {
            FAIL();
        }
        LOOSEo();

    case ICI_TRI(TC_STRING, TC_STRING, T_LESS):
    case ICI_TRI(TC_STRING, TC_STRING, T_GRT):
    case ICI_TRI(TC_STRING, TC_STRING, T_LESSEQ):
    case ICI_TRI(TC_STRING, TC_STRING, T_GRTEQ):
        {
            ssize_t   compare;
            str   *s1;
            str   *s2;

            s1 = stringof(o0);
            s2 = stringof(o1);
            compare = s1->s_nchars < s2->s_nchars ? s1->s_nchars : s2->s_nchars;
            compare = memcmp(s1->s_chars, s2->s_chars, compare);
            if (compare == 0) {
                if (s1->s_nchars < s2->s_nchars) {
                    compare = -1;
                } else if (s1->s_nchars > s2->s_nchars) {
                    compare = 1;
                }
            }
            switch (opof(o)->op_code) {
            case t_subtype(T_LESS):   if (compare < 0) USE1(); break;
            case t_subtype(T_GRT):    if (compare > 0) USE1(); break;
            case t_subtype(T_LESSEQ): if (compare <= 0) USE1(); break;
            case t_subtype(T_GRTEQ):  if (compare >= 0) USE1(); break;
            }
        }
        USE0();

    case ICI_TRI(TC_PTR, TC_PTR, T_LESS):
        if (!isint(ptrof(o1)->p_key) || !isint(ptrof(o0)->p_key)) {
            MISMATCH();
        }
        if (intof(ptrof(o0)->p_key)->i_value < intof(ptrof(o1)->p_key)->i_value) {
            USE1();
        }
        USE0();

    case ICI_TRI(TC_PTR, TC_PTR, T_GRTEQ):
        if (!isint(ptrof(o1)->p_key) || !isint(ptrof(o0)->p_key)) {
            MISMATCH();
        }
        if (intof(ptrof(o0)->p_key)->i_value >=intof(ptrof(o1)->p_key)->i_value) {
            USE1();
        }
        USE0();

    case ICI_TRI(TC_PTR, TC_PTR, T_LESSEQ):
        if (!isint(ptrof(o1)->p_key) || !isint(ptrof(o0)->p_key)) {
            MISMATCH();
        }
        if (intof(ptrof(o0)->p_key)->i_value <=intof(ptrof(o1)->p_key)->i_value) {
            USE1();
        }
        USE0();

    case ICI_TRI(TC_PTR, TC_PTR, T_GRT):
        if (!isint(ptrof(o1)->p_key) || !isint(ptrof(o0)->p_key)) {
            MISMATCH();
        }
        if (intof(ptrof(o0)->p_key)->i_value > intof(ptrof(o1)->p_key)->i_value) {
            USE1();
        }
        USE0();

    case ICI_TRI(TC_VEC32, TC_VEC32, T_PLUS):
    case ICI_TRI(TC_VEC32, TC_VEC32, T_PLUSEQ):
        MATCHVEC(vec32of);
        (*vec32of(o0)) += (*vec32of(o1));
        o = o0;
        USEo();

    case ICI_TRI(TC_VEC32, TC_VEC32, T_MINUS):
    case ICI_TRI(TC_VEC32, TC_VEC32, T_MINUSEQ):
        MATCHVEC(vec32of);
        (*vec32of(o0)) -= (*vec32of(o1));
        o = o0;
        USEo();

    case ICI_TRI(TC_VEC32, TC_VEC32, T_ASTERIX):
    case ICI_TRI(TC_VEC32, TC_VEC32, T_ASTERIXEQ):
        MATCHVEC(vec32of);
        (*vec32of(o0)) *= (*vec32of(o1));
        o = o0;
        USEo();

    case ICI_TRI(TC_VEC32, TC_VEC32, T_SLASH):
    case ICI_TRI(TC_VEC32, TC_VEC32, T_SLASHEQ):
        MATCHVEC(vec32of);
        (*vec32of(o0)) /= (*vec32of(o1));
        o = o0;
        USEo();

    case ICI_TRI(TC_VEC64, TC_VEC64, T_PLUS):
    case ICI_TRI(TC_VEC64, TC_VEC64, T_PLUSEQ):
        MATCHVEC(vec64of);
        (*vec64of(o0)) += (*vec64of(o1));
        o = o0;
        USEo();

    case ICI_TRI(TC_VEC64, TC_VEC64, T_MINUS):
    case ICI_TRI(TC_VEC64, TC_VEC64, T_MINUSEQ):
        MATCHVEC(vec64of);
        (*vec64of(o0)) -= (*vec64of(o1));
        o = o0;
        USEo();

    case ICI_TRI(TC_VEC64, TC_VEC64, T_ASTERIX):
    case ICI_TRI(TC_VEC64, TC_VEC64, T_ASTERIXEQ):
        MATCHVEC(vec64of);
        (*vec64of(o0)) *= (*vec64of(o1));
        o = o0;
        USEo();

    case ICI_TRI(TC_VEC64, TC_VEC64, T_SLASH):
    case ICI_TRI(TC_VEC64, TC_VEC64, T_SLASHEQ):
        MATCHVEC(vec64of);
        (*vec64of(o0)) /= (*vec64of(o1));
        o = o0;
        USEo();

    case ICI_TRI(TC_VEC32, TC_INT, T_PLUS):
    case ICI_TRI(TC_VEC32, TC_INT, T_PLUSEQ):
        (*vec32of(o0)) += intof(o1)->i_value;
        o = o0;
        USEo();

    case ICI_TRI(TC_VEC32, TC_INT, T_MINUS):
    case ICI_TRI(TC_VEC32, TC_INT, T_MINUSEQ):
        (*vec32of(o0)) -= intof(o1)->i_value;
        o = o0;
        USEo();

    case ICI_TRI(TC_VEC32, TC_INT, T_ASTERIX):
    case ICI_TRI(TC_VEC32, TC_INT, T_ASTERIXEQ):
        (*vec32of(o0)) *= intof(o1)->i_value;
        o = o0;
        USEo();

    case ICI_TRI(TC_VEC32, TC_INT, T_SLASH):
    case ICI_TRI(TC_VEC32, TC_INT, T_SLASHEQ):
        (*vec32of(o0)) /= intof(o1)->i_value;
        o = o0;
        USEo();

    case ICI_TRI(TC_VEC32, TC_FLOAT, T_PLUS):
    case ICI_TRI(TC_VEC32, TC_FLOAT, T_PLUSEQ):
        (*vec32of(o0)) += floatof(o1)->f_value;
        o = o0;
        USEo();

    case ICI_TRI(TC_VEC32, TC_FLOAT, T_MINUS):
    case ICI_TRI(TC_VEC32, TC_FLOAT, T_MINUSEQ):
        (*vec32of(o0)) -= floatof(o1)->f_value;
        o = o0;
        USEo();

    case ICI_TRI(TC_VEC32, TC_FLOAT, T_ASTERIX):
    case ICI_TRI(TC_VEC32, TC_FLOAT, T_ASTERIXEQ):
        (*vec32of(o0)) *= floatof(o1)->f_value;
        o = o0;
        USEo();

    case ICI_TRI(TC_VEC32, TC_FLOAT, T_SLASH):
    case ICI_TRI(TC_VEC32, TC_FLOAT, T_SLASHEQ):
        (*vec32of(o0)) /= floatof(o1)->f_value;
        o = o0;
        USEo();

    case ICI_TRI(TC_VEC64, TC_INT, T_PLUS):
    case ICI_TRI(TC_VEC64, TC_INT, T_PLUSEQ):
        (*vec64of(o0)) += intof(o1)->i_value;
        o = o0;
        USEo();

    case ICI_TRI(TC_VEC64, TC_INT, T_MINUS):
    case ICI_TRI(TC_VEC64, TC_INT, T_MINUSEQ):
        (*vec64of(o0)) -= intof(o1)->i_value;
        o = o0;
        USEo();

    case ICI_TRI(TC_VEC64, TC_INT, T_ASTERIX):
    case ICI_TRI(TC_VEC64, TC_INT, T_ASTERIXEQ):
        (*vec64of(o0)) *= intof(o1)->i_value;
        o = o0;
        USEo();

    case ICI_TRI(TC_VEC64, TC_INT, T_SLASH):
    case ICI_TRI(TC_VEC64, TC_INT, T_SLASHEQ):
        (*vec64of(o0)) /= intof(o1)->i_value;
        o = o0;
        USEo();

    case ICI_TRI(TC_VEC64, TC_FLOAT, T_PLUS):
    case ICI_TRI(TC_VEC64, TC_FLOAT, T_PLUSEQ):
        (*vec64of(o0)) += floatof(o1)->f_value;
        o = o0;
        USEo();

    case ICI_TRI(TC_VEC64, TC_FLOAT, T_MINUS):
    case ICI_TRI(TC_VEC64, TC_FLOAT, T_MINUSEQ):
        (*vec64of(o0)) -= floatof(o1)->f_value;
        o = o0;
        USEo();

    case ICI_TRI(TC_VEC64, TC_FLOAT, T_ASTERIX):
    case ICI_TRI(TC_VEC64, TC_FLOAT, T_ASTERIXEQ):
        (*vec64of(o0)) *= floatof(o1)->f_value;
        o = o0;
        USEo();

    case ICI_TRI(TC_VEC64, TC_FLOAT, T_SLASH):
    case ICI_TRI(TC_VEC64, TC_FLOAT, T_SLASHEQ):
        (*vec64of(o0)) /= floatof(o1)->f_value;
        o = o0;
        USEo();

    default:
//  others:
        switch (opof(o)->op_code) {
        case t_subtype(T_PLUSEQ):
	    if (o0->o_tcode == TC_SET) {
		if (ici_assign(o0, o1, o_one)) {
		    FAIL();
                }
		o = o0;
		USEo();
	    }
	    MISMATCH();

	case t_subtype(T_MINUSEQ):
	    if (o0->o_tcode == TC_SET) {
		if (unassign(setof(o0), o1)) {
		    FAIL();
                }
		o = o0;
		USEo();
	    }
	    MISMATCH();

        case t_subtype(T_EQEQ):
            if (o0->icitype() == o1->icitype() && compare(o0, o1) == 0) {
                USE1();
            }
            USE0();

        case t_subtype(T_EXCLAMEQ):
            if (!(o0->icitype() == o1->icitype() && compare(o0, o1) == 0)) {
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
                objname(n1, o0),
                binop_name(opof(o)->op_code),
                objname(n2, o1));
        }
        set_error(buf);
        FAIL();

    vecmismatch:
        // nb. the cast works for vec64 too
        set_error("vec size mis-match: %lu vs. %lu", vec32of(o0)->v_capacity, vec32of(o1)->v_capacity);
        FAIL();
    }

#ifdef BINOPFUNC
use0:
    os.a_top[-2] = o_zero;
    goto done;

use1:
    os.a_top[-2] = o_one;
    goto done;
#endif

usef:
    if (can_temp) {
        ptrdiff_t n;

        n = &os.a_top[-2] - os.a_base;
        if (ex->x_os_temp_cache->stk_probe(n)) {
            FAIL();
        }
        if ((o = ex->x_os_temp_cache->a_base[n]) == null) {
            if ((o = reinterpret_cast<object *>(ici_talloc(ostemp))) == nullptr) {
                FAIL();
            }
            ex->x_os_temp_cache->a_base[n] = o;
            rego(o);
        }
        set_tfnz(o, TC_FLOAT, object::O_TEMP, 0, sizeof (ostemp));
        floatof(o)->f_value = f;
        USEo();
    }
    /*
     * The following in-line expansion of float creation replaces, and
     * this should be equivalent to, this old code:
     *
     * if ((o = new_float(f)) == nullptr)
     *     FAIL();
     * LOOSEo();
     */

    /*
     * In-line expansion of float creation.
     */
    {
        object               **po;
        unsigned long           h;

#if 1
        h = hash_float(f);
#else
        union
        {
            double              f;
            int32_t             l[2];
        }
            v;

        /*
         * If this assert fails, we should switch back to a an explicit call
         * to hash_float().
         */
        assert(sizeof floatof(o)->f_value == 2 * sizeof (int32_t));
        v.f = f;
        h = FLOAT_PRIME + v.l[0] + v.l[1] * 31;
        h ^= (h >> 12) ^ (h >> 24);
#endif

        for
        (
            po = &atoms[atom_hash_index(h)];
            (o = *po) != nullptr;
            --po < atoms ? po = atoms + atomsz - 1 : nullptr
        ) {
#if 1
            if (isfloat(o) && DBL_BIT_CMP(&floatof(o)->f_value, &f))
#else
            if (isfloat(o) && DBL_BIT_CMP(&floatof(o)->f_value, &v.f))
#endif
            {
                USEo();
            }
        }
        ++supress_collect;
        if ((o = ici_talloc(ici_float)) == nullptr) {
            --supress_collect;
            FAIL();
        }
        set_tfnz(o, TC_FLOAT, object::O_ATOM, 1, sizeof (ici_float));
        floatof(o)->f_value = f;
        rego(o);
        assert(h == hashof(o));
        --supress_collect;
        store_atom_and_count(po, o);
        LOOSEo();
    }

usei:
    if (can_temp) {
        ptrdiff_t   n;

        n = &os.a_top[-2] - os.a_base;
        if (UNLIKELY(ex->x_os_temp_cache->stk_probe(n))) {
            FAIL();
        }
        if ((o = ex->x_os_temp_cache->a_base[n]) == null) {
            if ((o = reinterpret_cast<object *>(ici_talloc(ostemp))) == nullptr) {
                FAIL();
            }
            ex->x_os_temp_cache->a_base[n] = o;
            rego(o);
        }
        set_tfnz(o, TC_INT, object::O_TEMP, 0, sizeof (ostemp));
        intof(o)->i_value = i;
        USEo();
    }
    /*
     * In-line expansion of atom_int() from object.c. Following that, and
     * merged with it, is in-line atom creation.
     */
    if ((i & ~small_int_mask) == 0) {
        o = small_ints[i];
        USEo();
    }
    {
        object      **po;

        for
        (
            po = &atoms[atom_hash_index((unsigned long)i * INT_PRIME)];
            (o = *po) != nullptr;
            --po < atoms ? po = atoms + atomsz - 1 : nullptr
        )
        {
            if (isint(o) && intof(o)->i_value == i) {
                USEo();
            }
        }
        ++supress_collect;
        if ((o = ici_talloc(integer)) == nullptr) {
            --supress_collect;
            FAIL();
        }
        set_tfnz(o, TC_INT, object::O_ATOM, 1, sizeof (integer));
        intof(o)->i_value = i;
        rego(o);
        --supress_collect;
        store_atom_and_count(po, o);
    }

#ifdef BINOPFUNC
looseo:
    decref(o);
useo:
    os.a_top[-2] = o;
done:
#else // non-binop func version does not 'goto' the labels above
    decref(o);
    os.a_top[-2] = o;
#endif
    --os.a_top;
    /*--xs.a_top; Don't do this because it has been pre-done. */
}
