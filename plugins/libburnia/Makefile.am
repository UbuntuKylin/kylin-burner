
AM_CPPFLAGS = \
	-I$(top_srcdir)					\
	-I$(top_srcdir)/libburner-media/					\
	-I$(top_builddir)/libburner-media/		\
	-I$(top_srcdir)/libburner-burn				\
	-I$(top_builddir)/libburner-burn/				\
	-DBURNER_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" 	\
	-DBURNER_PREFIX=\"$(prefix)\"           		\
	-DBURNER_SYSCONFDIR=\"$(sysconfdir)\"   		\
	-DBURNER_DATADIR=\"$(datadir)/burner\"     	    	\
	-DBURNER_LIBDIR=\"$(libdir)\"  	         	\
	$(WARN_CFLAGS)							\
	$(DISABLE_DEPRECATED)				\
	$(BURNER_LIBISOFS_CFLAGS)			\
	$(BURNER_LIBBURN_CFLAGS)			\
	$(BURNER_GLIB_CFLAGS)

#libburn
libburndir = $(BURNER_PLUGIN_DIRECTORY)
libburn_LTLIBRARIES = libburner-libburn.la
libburner_libburn_la_SOURCES = burn-libburn.c 		\
				burn-libburn-common.c	\
				burn-libburn-common.h  \
				burn-libburnia.h 
libburner_libburn_la_LIBADD = ../../libburner-burn/libburner-burn3.la $(BURNER_GLIB_LIBS) $(BURNER_LIBBURNIA_LIBS)
libburner_libburn_la_LDFLAGS = -module -avoid-version

#libisofs (apparently libisofs needs one libburn function)
libisofsdir = $(BURNER_PLUGIN_DIRECTORY)
libisofs_LTLIBRARIES = libburner-libisofs.la
libburner_libisofs_la_SOURCES = burn-libisofs.c                       \
	burn-libburn-common.c burn-libburn-common.h			\
	burn-libburnia.h 
libburner_libisofs_la_LIBADD = ../../libburner-burn/libburner-burn3.la $(BURNER_GLIB_LIBS) $(BURNER_LIBBURNIA_LIBS)
libburner_libisofs_la_LDFLAGS = -module -avoid-version

-include $(top_srcdir)/git.mk
