
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

#audio2cue
audio2cuedir = $(BURNER_PLUGIN_DIRECTORY)
audio2cue_LTLIBRARIES = libburner-audio2cue.la
libburner_audio2cue_la_SOURCES = burn-audio2cue.c
libburner_audio2cue_la_LIBADD = ../../libburner-burn/libburner-burn3.la $(BURNER_GLIB_LIBS)
libburner_audio2cue_la_LDFLAGS = -module -avoid-version

-include $(top_srcdir)/git.mk
