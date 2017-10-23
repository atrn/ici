#ifndef ICI_UTYPE_H
#define ICI_UTYPE_H

#include <ici.h>

/*
 * The type code for utype and ustruct.
 */
extern int              ici_utype_tcode;
extern int              ici_ustruct_tcode;

/*
 * Each utype is represented by the following structure...
 */
typedef struct ici_utype
{
    ici_obj_t	o_head;
    ici_str_t	*u_name;
    ici_func_t  *u_new;
    ici_func_t  *u_assign;
    ici_func_t  *u_fetch;
    ici_type_t  u_type;
}
ici_utype_t;
/*
 * o_head	The usual ICI object header.
 * u_name	The name of the type. We keep a reference to this
 *		string object so the ici_type_t can point directly
 *		at the C string within the string object.
 * u_new	Pointer to constructor function that is called when an
 *		object of this type is created via the new() function.
 *		The function is passed one or more parameters, the first
 *		being the struct used to store data for this object and
 *		the subsequent parameters are any passed to the new()
 *		function. See f_new() below. If this is NULL then there
 *		is no new function and nothing is done other than normal
 *		ICI struct object initialisation.
 * u_assign	Pointer to assignment function. This function is called
 *		when a sub-object of the object is being assigned to.
 *		The function is called with three parameters, the struct
 *		that stores the object's data and the key and value. The
 *		function then controls if the value is assigned to key
 *		within the struct. If there is no assign function the
 *		normal struct object assign function is used which allows
 *		any type of key and value to be assigned.
 * u_fetch	Pointer to fetch function. This function is called to return
 *		objects given a key. The ICI function is called with two
 *		parameters, the struct for this object instance and the
 *		key. The function returns the value associated with that
 *		key. This may simply involve looking the key up in the
 *		struct or may require computation or even I/O. If no
 *		fetch function is specified this is NULL and the normal
 *		struct object fetch function is used. This simply returns
 *		the value at a particular key.
 * u_type	The type_t structure for this type. This is created when the
 *		type is defined via the def() function and is referenced
 *		by objects of this type.
 */

#define	ici_utypeof(o)	((ici_utype_t *)o)
#define	ici_isutype(o)	((o)->o_tcode == ici_utype_tcode)

/*
 * Each instance of a user type is represented by a ustruct. This is
 * simply a structure that associates a struct object with a reference
 * to a utype_t defining the fetch and assign operations for the struct.
 */
typedef struct ici_ustruct
{
    ici_obj_t	        o_head;
    ici_struct_t	*u_struct;
    ici_utype_t         *u_type;
}
ici_ustruct_t;
/*
 * o_head	The usual ICI object header.
 * u_struct	The struct that stores the key/values for this type.
 * u_type	The utype_t for objects of this type. This lets us
 *		find the fetch/assign functions when required.
 */

#define ici_ustructof(o)	((ici_ustruct_t *)(o))

#endif /* #ifndef ICI_UTYPE_H */
