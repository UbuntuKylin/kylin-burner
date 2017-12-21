
AM_CPPFLAGS = \
	-I$(top_srcdir)						\
	-I$(top_srcdir)/libburner-media/					\
	-I$(top_builddir)/libburner-media/			\
	-I$(top_srcdir)/libburner-burn				\
	-I$(top_builddir)/libburner-burn/				\
	-DBURNER_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" 	\
	-DBURNER_PREFIX=\"$(prefix)\"           		\
	-DBURNER_SYSCONFDIR=\"$(sysconfdir)\"   		\
	-DBURNER_DATADIR=\"$(datadir)/burner\"     	    	\
	-DBURNER_LIBDIR=\"$(libdir)\"  	         	\
	$(WARN_CFLAGS)							\
	$(DISABLE_DEPRECATED)					\
	$(BURNER_GLIB_CFLAGS)					\
	$(BURNER_GIO_CFLAGS)

#local-track
local_trackdir = $(BURNER_PLUGIN_DIRECTORY)
local_track_LTLIBRARIES = libburner-local-track.la

libburner_local_track_la_SOURCES = burn-local-image.c 
libburner_local_track_la_LIBADD = ../../libburner-burn/libburner-burn3.la $(BURNER_GLIB_LIBS) $(BURNER_GIO_LIBS)
libburner_local_track_la_LDFLAGS = -module -avoid-version

#burn-uri
burn_uridir = $(BURNER_PLUGIN_DIRECTORY)
burn_uri_LTLIBRARIES = libburner-burn-uri.la

libburner_burn_uri_la_SOURCES = burn-uri.c 
libburner_burn_uri_la_LIBADD = ../../libburner-burn/libburner-burn3.la $(BURNER_GLIB_LIBS) $(BURNER_GIO_LIBS)
libburner_burn_uri_la_LDFLAGS = -module -avoid-version

-include $(top_srcdir)/git.mk
