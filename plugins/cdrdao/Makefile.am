
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

#cdrdao
cdrdaodir = $(BURNER_PLUGIN_DIRECTORY)
cdrdao_LTLIBRARIES = libburner-cdrdao.la
libburner_cdrdao_la_SOURCES = burn-cdrdao.c 
libburner_cdrdao_la_LIBADD = ../../libburner-media/libburner-media3.la ../../libburner-burn/libburner-burn3.la $(BURNER_GLIB_LIBS)
libburner_cdrdao_la_LDFLAGS = -module -avoid-version

-include $(top_srcdir)/git.mk
