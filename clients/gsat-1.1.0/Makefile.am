## Process this file with automake to produce Makefile.in

SUBDIRS = intl po src

install-data-local:
	@$(NORMAL_INSTALL)
	if test -d $(srcdir)/pixmaps; then \
	  $(mkinstalldirs) $(DESTDIR)$(pkgdatadir)/pixmaps; \
	  for pixmap in $(srcdir)/pixmaps/*; do \
	    if test -f $$pixmap; then \
	      $(INSTALL_DATA) $$pixmap $(DESTDIR)$(pkgdatadir)/pixmaps; \
	    fi \
	  done \
	fi
	if test ! -d $(pkglibdir)/plugins; then \
	  $(mkinstalldirs) $(pkglibdir)/plugins; \
	fi
	if test ! -f $(pkglibdir)/gsat.prefs; then \
	  $(INSTALL_DATA) gsat.prefs $(pkglibdir)/; \
	fi
	if test ! -f $(pkglibdir)/gsat.db; then \
	  $(INSTALL_DATA) gsat.db $(pkglibdir)/; \
	fi

dist-hook:
	if test -d pixmaps; then \
	  mkdir $(distdir)/pixmaps; \
	  for pixmap in pixmaps/*; do \
	    if test -f $$pixmap; then \
	      cp -p $$pixmap $(distdir)/pixmaps; \
	    fi \
	  done \
	fi

