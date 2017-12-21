
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

#cdrecord
cdrecorddir = $(BURNER_PLUGIN_DIRECTORY)
cdrecord_LTLIBRARIES = libburner-cdrecord.la
libburner_cdrecord_la_SOURCES = burn-cdrecord.c \
	burn-cdrtools.h 
libburner_cdrecord_la_LIBADD = ../../libburner-burn/libburner-burn3.la $(BURNER_GLIB_LIBS)
libburner_cdrecord_la_LDFLAGS = -module -avoid-version

#mkisofs
mkisofsdir = $(BURNER_PLUGIN_DIRECTORY)
mkisofs_LTLIBRARIES = libburner-mkisofs.la
libburner_mkisofs_la_SOURCES = burn-mkisofs.c \
	burn-cdrtools.h 
libburner_mkisofs_la_LIBADD = ../../libburner-burn/libburner-burn3.la $(BURNER_GLIB_LIBS)
libburner_mkisofs_la_LDFLAGS = -module -avoid-version

#readcd
readcddir = $(BURNER_PLUGIN_DIRECTORY)
readcd_LTLIBRARIES = libburner-readcd.la
libburner_readcd_la_SOURCES = burn-readcd.c \
	burn-cdrtools.h 
libburner_readcd_la_LIBADD = ../../libburner-media/libburner-media3.la ../../libburner-burn/libburner-burn3.la $(BURNER_GLIB_LIBS)
libburner_readcd_la_LDFLAGS = -module -avoid-version

#cdda2wav
cdda2wavdir = $(BURNER_PLUGIN_DIRECTORY)
cdda2wav_LTLIBRARIES = libburner-cdda2wav.la
libburner_cdda2wav_la_SOURCES = burn-cdda2wav.c burn-cdrtools.h 
libburner_cdda2wav_la_LIBADD = ../../libburner-media/libburner-media3.la ../../libburner-burn/libburner-burn3.la $(BURNER_GLIB_LIBS)
libburner_cdda2wav_la_LDFLAGS = -module -avoid-version

-include $(top_srcdir)/git.mk
