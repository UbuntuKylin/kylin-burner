
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
	$(BURNER_GLIB_CFLAGS)				\
	$(BURNER_GSTREAMER_CFLAGS)

transcodedir = $(BURNER_PLUGIN_DIRECTORY)
transcode_LTLIBRARIES = libburner-transcode.la

libburner_transcode_la_SOURCES = burn-transcode.c burn-normalize.h 
libburner_transcode_la_LIBADD = ../../libburner-burn/libburner-burn3.la $(BURNER_GLIB_LIBS) $(BURNER_GSTREAMER_LIBS)
libburner_transcode_la_LDFLAGS = -module -avoid-version

normalizedir = $(BURNER_PLUGIN_DIRECTORY)
normalize_LTLIBRARIES = libburner-normalize.la

libburner_normalize_la_SOURCES = burn-normalize.c burn-normalize.h
libburner_normalize_la_LIBADD = ../../libburner-burn/libburner-burn3.la $(BURNER_GLIB_LIBS) $(BURNER_GSTREAMER_LIBS)
libburner_normalize_la_LDFLAGS = -module -avoid-version

vobdir = $(BURNER_PLUGIN_DIRECTORY)
vob_LTLIBRARIES = libburner-vob.la

libburner_vob_la_SOURCES = burn-vob.c 
libburner_vob_la_LIBADD = ../../libburner-burn/libburner-burn3.la $(BURNER_GLIB_LIBS) $(BURNER_GSTREAMER_LIBS)
libburner_vob_la_LDFLAGS = -module -avoid-version

-include $(top_srcdir)/git.mk
