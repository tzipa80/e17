MAINTAINERCLEANFILES = Makefile.in

AM_CPPFLAGS = \
-I. \
-I$(top_srcdir)/src/lib \
-I$(top_srcdir)/src/lib/include \
-I$(top_srcdir)/src/lib/cserve \
-DPACKAGE_BIN_DIR=\"$(bindir)\" \
-DPACKAGE_LIB_DIR=\"$(libdir)\" \
-DPACKAGE_DATA_DIR=\"$(datadir)/$(PACKAGE)\" \
@EVAS_GENERAL_CFLAGS@ \
@FREETYPE_CFLAGS@ \
@FRIBIDI_CFLAGS@ \
@EET_CFLAGS@ \
@FONTCONFIG_CFLAGS@ \
@pthread_cflags@ \
@PIXMAN_CFLAGS@

if EVAS_CSERVE

noinst_LTLIBRARIES = libevas_cserve.la

endif

libevas_cserve_la_SOURCES = \
evas_cs.h \
evas_cs_main.c \
evas_cs_server.c \
evas_cs_client.c \
evas_cs_mem.c

libevas_cserve_la_LIBADD = @EVAS_GENERAL_LIBS@ @EFL_SHM_OPEN_LIBS@
