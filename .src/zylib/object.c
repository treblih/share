/*
 * =====================================================================================
 *
 *       Filename:  object.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  16.03.10
 *       Revision:  
 *       Compiler:  GCC 4.4.3
 *
 *         Author:  Yang Zhang, imyeyeslove@gmail.com
 *        Company:  
 *
 * =====================================================================================
 */
#define		OBJECT_IMPLEMENTATION

#include	"object.h"
#include	"object.r"

#define		offsetof(type, member)		__builtin_offsetof(type, member)

static void *obj_ctor(void *, va_list *);
static void *obj_dtor(void *);
static void *cla_ctor(void *, va_list *);
static void *cla_dtor(void *);

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  class_of
 *  Description:  class of a given object
 *
 *  		  the only thing that one object always have is a pointer to it's class
 * =====================================================================================
 */
const void *class_of(const void * _self)
{
	const struct object *self = _self;	/* upcasting */
	return self->class;
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  super_of
 *  Description:  a class' super class
 * =====================================================================================
 */
const void *super_of(const void *_cla)
{
	const struct class *cla = _cla;
	return cla->super;
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  size_of
 *  Description:  a object may not have it's size element, so can't "->size" directly
 *  		  but it was generated by it's class-desc, so it has the record of
 *  		  the object
 * =====================================================================================
 */
size_t size_of(void *_self)
{
	const struct class *cla = class_of(_self);
	return cla->size;
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  name_of
 *  Description:  the class' name
 * =====================================================================================
 */
const char *name_of(void *_cla)
{
	const struct class *cla = _cla;
	return cla->name;
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  ctor
 *  Description:  i thought that ctor ought to receive a class pointer, not a object one
 *   		  for we just know the info about a class and use it to instance an obj
 *   		  why could we g..
 * =====================================================================================
 */
void *ctor(void *_self, va_list * arg)
{
	const struct class *cla = class_of(_self);
	return cla->ctor(_self, arg);                   /* no need to dereference arg */
}

void *dtor(void *_self)
{
	const struct class *cla = class_of(_self);
	return cla->dtor(_self);
}

void *super_ctor(const void * _spr, void *_self, va_list * arg)
{
	const struct class *spr_cla = super_of(_spr);
	/*-----------------------------------------------------------------------------
	 *  return the super-ctor's retval, 
	 *  ctor always return the modifying desc, so wouldn't be "const"
	 *-----------------------------------------------------------------------------*/
	return spr_cla->ctor(_self, arg);
}

void *super_dtor(void *_self)
{
	struct class *cla = super_of(_self);
	return cla->dtor(_self);
}

static void *obj_ctor(void *_self, va_list * arg)
{
	return _self;
}

static void *obj_dtor(void *_self)
{
	return _self;
}

static void *cla_ctor(void *_new_cla, va_list * arg)
{
	struct class *cls = _new_cla;
	va_list ap = *arg;                              /* use so dereference */

	cls->super = va_arg(ap, struct class *);
	cls->name = va_arg(ap, const char *);
	cls->size = va_arg(ap, size_t);

	/*-----------------------------------------------------------------------------
	 *  method offset is certain
	 *-----------------------------------------------------------------------------*/
	int method_off = offsetof(struct class, ctor);

	/*-----------------------------------------------------------------------------
	 *  we wanna know the method length of this constructing one's super class,
	 *  so need to ask the super class->class->size
	 *-----------------------------------------------------------------------------*/
	memcpy((char *)cls + method_off, (char *)cls->super + method_off, size_of(cls->super) - method_off);

	typedef void * (* funcp) ();
	funcp selector;

	/*-----------------------------------------------------------------------------
	 *  need optimization here
	 *-----------------------------------------------------------------------------*/
	while ((selector = va_arg(ap, funcp))) {
		if (selector == ctor) {
			funcp method = va_arg(ap, funcp);
			cls->ctor = method;
		} else if (selector == dtor) {
			funcp method = va_arg(ap, funcp);
			cls->dtor = method;
		}
	}
	
	return cls;
}

static void *cla_dtor(void *_cla)
{
	struct class * cla = _cla;
	fprintf(stderr, "%s: cannot delete class\n", cla->name);
	return 0;
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  new
 *  Description:  new == calloc(allocate space in heap) + ctor(init)
 *
 *  		  must be a class pointer
 * =====================================================================================
 */
void *new(const void *_cla, ...)
{
	va_list arg;
	const struct class *cla = _cla;
	struct object *obj;

	obj = calloc(1, cla->size);
	obj->class = cla;	/* just 1 component */

	va_start(arg, _cla);
	obj = ctor(obj, &arg);
	va_end(arg);

	return obj;
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  delete
 *  Description:  just free. 'cause no need to finit
 * =====================================================================================
 */
void delete(void * _self)
{
	struct class * cla = _self;
	if (_self)
		free(cla->dtor(_self));
}

/*-----------------------------------------------------------------------------
 *  obj ->	a class, pointed by "object", see below
 *  obj + 1 ->	a class, pointed by "class", see below
 *
 *  for struct class has the member "object", so need the extra {}
 *-----------------------------------------------------------------------------*/
static struct class obj[2] = {
	{ {obj + 1 }, obj, "Object", sizeof(struct object), obj_ctor, obj_dtor },
	{ {obj + 1 }, obj, "Class", sizeof(struct class), cla_ctor, cla_dtor }
};

const void *object = obj;
const void *class = obj + 1;
