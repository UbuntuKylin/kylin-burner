
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
	$(BURNER_GLIB_CFLAGS)

#growisofs
growisofsdir = $(BURNER_PLUGIN_DIRECTORY)
growisofs_LTLIBRARIES = libburner-growisofs.la
libburner_growisofs_la_SOURCES = burn-growisofs.c \
	burn-growisofs-common.h 
libburner_growisofs_la_LIBADD = ../../libburner-burn/libburner-burn3.la $(BURNER_GLIB_LIBS)
libburner_growisofs_la_LDFLAGS = -module -avoid-version

#dvdrwformat
dvdrwformatdir = $(BURNER_PLUGIN_DIRECTORY)
dvdrwformat_LTLIBRARIES = libburner-dvdrwformat.la
libburner_dvdrwformat_la_SOURCES = burn-dvd-rw-format.c 
libburner_dvdrwformat_la_LIBADD = ../../libburner-burn/libburner-burn3.la $(BURNER_GLIB_LIBS)
libburner_dvdrwformat_la_LDFLAGS = -module -avoid-version

-include $(top_srcdir)/git.mk
