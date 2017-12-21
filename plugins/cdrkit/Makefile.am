
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

#wodim
wodimdir = $(BURNER_PLUGIN_DIRECTORY)
wodim_LTLIBRARIES = libburner-wodim.la
libburner_wodim_la_SOURCES = burn-wodim.c \
	burn-cdrkit.h 
libburner_wodim_la_LIBADD = ../../libburner-media/libburner-media3.la ../../libburner-burn/libburner-burn3.la $(BURNER_GLIB_LIBS)
libburner_wodim_la_LDFLAGS = -module -avoid-version

#genisoimage
genisoimagedir = $(BURNER_PLUGIN_DIRECTORY)
genisoimage_LTLIBRARIES = libburner-genisoimage.la
libburner_genisoimage_la_SOURCES = burn-genisoimage.c \
	burn-cdrkit.h 
libburner_genisoimage_la_LIBADD = ../../libburner-burn/libburner-burn3.la $(BURNER_GLIB_LIBS)
libburner_genisoimage_la_LDFLAGS = -module -avoid-version

#readom
readomdir = $(BURNER_PLUGIN_DIRECTORY)
readom_LTLIBRARIES = libburner-readom.la
libburner_readom_la_SOURCES = burn-readom.c \
	burn-cdrkit.h 
libburner_readom_la_LIBADD = ../../libburner-media/libburner-media3.la ../../libburner-burn/libburner-burn3.la $(BURNER_GLIB_LIBS)
libburner_readom_la_LDFLAGS = -module -avoid-version

-include $(top_srcdir)/git.mk
