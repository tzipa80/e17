/*
 * Copyright (C) 1997-2002, Michael Jennings
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies of the Software, its documentation and marketing & publicity
 * materials, and acknowledgment shall be given in the documentation, materials
 * and software packages that this Software was used.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef _LIBAST_IF_H_
#define _LIBAST_IF_H_

/*
 * interface goop
 */

/* Standard typecast macros.... */
#define SPIF_IFINST(obj)                           ((spif_ifinst_t) (obj))
#define SPIF_IFCLASS(cls)                          ((spif_ifclass_t) (cls))

/* Access the implementation class member of an object. */
#define SPIF_IFINST_IFCLASS(obj)                   (SPIF_IFINST(obj)->ifclass)

/* Call a method on an instance of an implementation class */
#define SPIF_IF_CALL_METHOD(obj, meth)             (SPIF_IFINST_IFCLASS(obj)->(meth))

/* Calls to the basic functions. */
#define SPIF_IFCLASS_NEW(cls)                      (SPIF_IFCLASS(cls)->new)
#define SPIF_IFINST_NEW(obj)                       (SPIF_IF_CALL_METHOD((obj), (new)))
#define SPIF_IFCLASS_INIT(cls)                     (SPIF_IFCLASS(cls)->init)
#define SPIF_IFINST_INIT(obj)                      (SPIF_IF_CALL_METHOD((obj), (init)))
#define SPIF_IFCLASS_DONE(cls)                     (SPIF_IFCLASS(cls)->done)
#define SPIF_IFINST_DONE(obj)                      (SPIF_IF_CALL_METHOD((obj), (done)))
#define SPIF_IFCLASS_DEL(cls)                      (SPIF_IFCLASS(cls)->del)
#define SPIF_IFINST_DEL(obj)                       (SPIF_IF_CALL_METHOD((obj), (del)))
#define SPIF_IFCLASS_SHOW(cls)                     (SPIF_IFCLASS(cls)->show)
#define SPIF_IFINST_SHOW(obj)                      (SPIF_IF_CALL_METHOD((obj), (show)))

/* The ifclass object contains the function pointers for the interface methods. */
typedef struct spif_ifclass_t_struct *spif_ifclass_t;
typedef struct spif_ifclass_t_struct spif_const_ifclass_t;

/* Generic function pointers. */
typedef spif_obj_t (*spif_newfunc_t)(void);
typedef spif_bool_t (*spif_memberfunc_t)(spif_obj_t);
typedef void * (*spif_func_t)();

struct spif_ifclass_t_struct {
  spif_newfunc_t new;
  spif_memberfunc_t init;
  spif_memberfunc_t done;
  spif_memberfunc_t del;
  spif_func_t show;
};

/* ifinst is the parent class for instances of interface classes. */
typedef struct spif_ifinst_t_struct *spif_ifinst_t;
typedef struct spif_ifinst_t_struct spif_const_ifinst_t;

struct spif_ifinst_t_struct {
  spif_obj_t parent;
  spif_ifclass_t ifclass;
};


#endif /* _LIBAST_IF_H_ */
