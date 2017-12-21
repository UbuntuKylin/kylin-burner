
AM_CPPFLAGS = \
	-I$(top_srcdir)							\
	-I$(top_srcdir)/libburner-media/					\
	-I$(top_builddir)/libburner-media/				\
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
	$(BURNER_GTK_CFLAGS)

checksumdir = $(BURNER_PLUGIN_DIRECTORY)
checksum_LTLIBRARIES = libburner-checksum.la
libburner_checksum_la_SOURCES = burn-checksum-image.c	

libburner_checksum_la_LDFLAGS = -module -avoid-version
libburner_checksum_la_LIBADD = ../../libburner-media/libburner-media3.la ../../libburner-burn/libburner-burn3.la $(BURNER_GLIB_LIBS)

checksumfiledir = $(BURNER_PLUGIN_DIRECTORY)
checksumfile_LTLIBRARIES = libburner-checksum-file.la
libburner_checksum_file_la_SOURCES = burn-checksum-files.c	\
				      burn-volume-read.c  \
				      burn-volume-read.h

libburner_checksum_file_la_LDFLAGS = -module -avoid-version
libburner_checksum_file_la_LIBADD = ../../libburner-media/libburner-media3.la ../../libburner-burn/libburner-burn3.la $(BURNER_GLIB_LIBS) $(BURNER_GTK_LIBS)

-include $(top_srcdir)/git.mk
