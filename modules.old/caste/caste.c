/*******************************************************
 * "CASTE" Module for ICI
 * Written by Tim Vernum
 * Based on work by Andy Newman
 *******************************************************
 *
 * This software, and associated materials are:
 *   Copyright (c) 2001, Tim Vernum.
 *   All rights reserved. 
 * 
 * Permission is hereby granted, free of charge, to any person 
 * obtaining a copy of this software and associated documentation 
 * files (the "Software"), to deal in the Software without 
 * restriction, including without limitation the rights to use, 
 * copy, modify, merge, publish, distribute, sublicense, and/or 
 * sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following 
 * conditions:
 * 
 * 1. Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer. 
 * 
 * 2. Redistributions in binary form must reproduce the above copyright 
 *    notice, this list of conditions and the following disclaimer in 
 *    the documentation and/or other materials provided with the 
 *    distribution.
 *    The phrasing "This software" in the copyright notice may
 *    optionally be replaced with "Portions of this software".
 * 
 * 3. The names of the author(s) and/or copyright holders must not be
 *    used to endorse or promote products derived from this software. 
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES 
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR 
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 *******************************************************/
 
#include "ici/ici.h"
#include "caste_strings.h"

extern int init_ici_str(void) ;

#define iscallable(o)  ( o->o_type->t_call != NULL )

extern type_t caste_type ;

static unsigned long mark_struct(object_t *o)
{
	return (*struct_type.t_mark)(o) ;
}

static void free_struct(object_t *o)
{
	return (*struct_type.t_free)(o) ;
}

static unsigned long hash_struct(object_t *o)
{
	return (*struct_type.t_hash)(o) ;
}

static int cmp_struct( object_t * a, object_t * b )
{
	return (*struct_type.t_cmp)(a,b) ;
}

static object_t * copy_struct( object_t * o )
{
	return (*struct_type.t_copy)(o) ;
}

static object_t * fetch_struct( object_t * o, object_t * k )
{
	return (*struct_type.t_fetch)(o,k) ;
}

static int assign_struct( object_t * o, object_t * k, object_t * v )
{
	return (*struct_type.t_assign)(o,k,v) ;
}

static int call_func( object_t * o, object_t * s, string_t * m )
{
	return (*ici_func_type.t_call)(o,s,m) ;
}

static unsigned long
mark_caste( object_t * o )
{
	return mark_struct(o) ;
}

static void
free_caste( object_t * o )
{
	free_struct(o) ;
}

static unsigned long
hash_caste( object_t * o )
{
	hash_struct(o) ;
}


static int
cmp_caste(object_t * a , object_t * b )
{
	return cmp_struct(a,b) ;
}

static object_t *
copy_caste( object_t * o )
{
	object_t * c ;
	c = copy_struct(o) ;
	if( c != NULL )
	{
		objof(c)->o_type = &caste_type ;
	}
	return c ;
}

static int
assign_caste(object_t *o, object_t *k, object_t *v)
{
	object_t * f ;
	method_t * m ;
	
	f = fetch_struct( o, ICISO(key_assign) ) ;
	if( !iscallable(f) )
	{
		return assign_struct(o,k,v) ;
	}

	m = ici_new_method( o, f ) ;
	error = ici_func( objof(m), "oo", k, v ) ;

	return ( error ? 1 : 0 ) ;
}

static object_t *
fetch_caste(object_t *o, object_t *k)
{
	object_t * f ;
	object_t * v ;
	method_t * m ;
	
	f = fetch_struct( o, ICISO(key_fetch) ) ;
	if( !iscallable(f) )
	{
		return fetch_struct(o,k) ;
	}

	m = ici_new_method( o, f ) ;
	error = ici_func( objof(m), "o=o", &v, k ) ;

	if ( error  )
		return NULL ;
	
	decref( v ) ;
	return v ;
}

static int
call_caste( object_t * o , object_t * s, string_t * m )
{
	object_t * f ;

	f = fetch_struct( o, ICISO(key_call) ) ;
	if( !iscallable(f) )
    {
        char n1[30];
        sprintf(buf, "Object (%s) is not callable - does not implement '%s' as a method", objname(n1, o), ICIS(key_call)->s_chars );
        error = buf;
        return 1;
    }   

	return call_func( f, o, m ) ;
}


type_t caste_type =
{
    mark_caste,
    free_caste,
    hash_caste,
    cmp_caste,
    copy_caste,
    assign_caste,
    fetch_caste,
    "caste",
    NULL,
    call_caste,
};

struct_t * new_caste()
{
	struct_t * c = new_struct() ;
	if (c != NULL)
	{
		objof(c)->o_type = &caste_type ;
	}
	return c ;
}

static int
f_caste_fetch(void)
{
	struct_t  * self ;
	object_t  * key ;
	object_t  * value ;

	if (ici_typecheck("do", &self, &key))
	    return 1;

	value = fetch_struct( objof(self), key ) ;
	
	return ici_ret_no_decref( value ) ;
}

static int
f_caste_assign(void)
{
	struct_t  * self ;
	object_t  * key ;
	object_t  * value ;

	if (ici_typecheck("doo", &self, &key, &value))
	{
	    return 1;
	}

	assign_struct( objof(self), key, value ) ;
	return null_ret() ;
}

static int
f_caste_new(void)
{
	struct_t * c ;
	
	if( NARGS() > 0 )
		return ici_argcount(0) ;
	
	c = new_caste() ;
	return ici_ret_with_decref( objof(c) ) ;
}

cfunc_t		caste_cfuncs[] =
{
    {CF_OBJ,	"caste_fetch"  ,		f_caste_fetch  },
    {CF_OBJ,	"caste_assign" ,		f_caste_assign },
    {CF_OBJ,	"caste_new"    ,		f_caste_new    },
    {CF_OBJ}
};

object_t *
ici_caste_library_init(void)
{
	struct_t	*s = new_struct();
	
	init_ici_str() ;
	
	if (s != NULL && ici_assign_cfuncs(s, caste_cfuncs))
	{
		decref(s);
		return NULL;
	}
	
	return objof(s);
}

/*
   String defining.
   It's a little verbose for my liking, but it works
 */
 
#undef  ICI_STR
#define ICI_STR ICI_STR_DECL
#include "caste_strings.h"

int
init_ici_str(void)
{
#undef  ICI_STR
#define ICI_STR ICI_STR_MAKE
    return
#include "caste_strings.h"
    0;
}
#undef  ICI_STR
#define ICI_STR ICI_STR_NORM
