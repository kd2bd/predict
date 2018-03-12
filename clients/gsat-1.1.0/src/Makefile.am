## Process this file with automake to produce Makefile.in

INCLUDES = \
	-I$(top_srcdir)/intl \
	@GTK_CFLAGS@

bin_PROGRAMS = gsat

gsat_SOURCES = \
	main.c \
	support.c support.h \
	interface.c interface.h \
	callbacks.c callbacks.h \
	comms.c comms.h \
	plugins.c plugins.h \
	db.c db.h \
	prefs.c prefs.h

gsat_LDADD = @GTK_LIBS@ $(INTLLIBS)

