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

static const char cvs_ident[] = "$Id$";

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <libast_internal.h>

static spif_uint32_t spif_socket_get_ipaddr(spif_socket_t);

/* *INDENT-OFF* */
static spif_const_class_t o_class = {
    SPIF_DECL_CLASSNAME(socket),
    (spif_newfunc_t) spif_socket_new,
    (spif_memberfunc_t) spif_socket_init,
    (spif_memberfunc_t) spif_socket_done,
    (spif_memberfunc_t) spif_socket_del,
    (spif_func_t) spif_socket_show,
    (spif_func_t) spif_socket_comp,
    (spif_func_t) spif_socket_dup,
    (spif_func_t) spif_socket_type
};
spif_class_t SPIF_CLASS_VAR(socket) = &o_class;
/* *INDENT-ON* */

spif_socket_t
spif_socket_new(void)
{
    spif_socket_t self;

    self = SPIF_ALLOC(socket);
    spif_socket_init(self);
    return self;
}

spif_socket_t
spif_socket_new_from_url(spif_url_t url)
{
    spif_socket_t self;

    self = SPIF_ALLOC(socket);
    spif_socket_init_from_url(self, url);
    return self;
}

spif_bool_t
spif_socket_del(spif_socket_t self)
{
    spif_socket_done(self);
    SPIF_DEALLOC(self);
    return TRUE;
}

spif_bool_t
spif_socket_init(spif_socket_t self)
{
    spif_obj_init(SPIF_OBJ(self));
    spif_obj_set_class(SPIF_OBJ(self), SPIF_CLASS_VAR(socket));
    return TRUE;
}

spif_bool_t
spif_socket_init_from_url(spif_socket_t self, spif_url_t url)
{
    spif_obj_init(SPIF_OBJ(self));
    spif_obj_set_class(SPIF_OBJ(self), SPIF_CLASS_VAR(socket));
    return TRUE;
}

spif_bool_t
spif_socket_done(spif_socket_t self)
{
    USE_VAR(self);
    return TRUE;
}

spif_str_t
spif_socket_show(spif_socket_t self, spif_charptr_t name, spif_str_t buff, size_t indent)
{
    char tmp[4096];

    memset(tmp, ' ', indent);
    snprintf(tmp + indent, sizeof(tmp) - indent, "(spif_socket_t) %s:  {\n", name);
    if (SPIF_STR_ISNULL(buff)) {
        buff = spif_str_new_from_ptr(tmp);
    } else {
        spif_str_append_from_ptr(buff, tmp);
    }
    return buff;
}

spif_cmp_t
spif_socket_comp(spif_socket_t self, spif_socket_t other)
{
    return (self == other);
}

spif_socket_t
spif_socket_dup(spif_socket_t self)
{
    spif_socket_t tmp;

    tmp = spif_socket_new();
    memcpy(tmp, self, SPIF_SIZEOF_TYPE(socket));
    return tmp;
}

spif_classname_t
spif_socket_type(spif_socket_t self)
{
    return SPIF_OBJ_CLASSNAME(self);
}

spif_bool_t
spif_socket_open(spif_socket_t self)
{

}

static spif_uint32_t
spif_socket_get_ipaddr(spif_socket_t self)
{
    struct hostent *hinfo;
    struct in_addr *addr;
    char *ipaddr_str;
    spif_str_t hostname;
    spif_uint8_t tries;

    REQUIRE_RVAL(!SPIF_SOCKET_ISNULL(self), 0);

    hostname = SPIF_STR(spif_url_get_host(SPIF_URL(self->dest_url)));
    REQUIRE_RVAL(!SPIF_STR_ISNULL(hostname), 0);

    h_errno = 0;
    tries = 0;
    do {
        tries++;
        hinfo = gethostbyname(SPIF_STR_STR(hostname));
    } while ((tries <= 3) && (hinfo == NULL) && (h_errno == TRY_AGAIN));
    if (hinfo == NULL) {
        print_error("Unable to resolve hostname \"%s\" -- %s\n", SPIF_STR_STR(hostname), hstrerror(h_errno));
        return 0;
    }

    ipaddr_str = hinfo->h_addr_list[0];
    addr = SPIF_CAST_C(struct in_addr *) MALLOC(sizeof(struct in_addr));
    if ((inet_aton(ipaddr_str, addr)) == 0) {
        print_error("Invalid address \"%s\" returned by gethostbyname()\n", NONULL(ipaddr_str));
        return 0;
    }

    return *(SPIF_CAST_PTR(uint32) (addr));
}
