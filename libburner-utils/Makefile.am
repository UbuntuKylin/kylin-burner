
AM_CPPFLAGS = \
	-I$(top_srcdir)							\
	-I$(top_builddir)						\
	-DBURNER_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" 	\
	-DBURNER_PREFIX=\"$(prefix)\"           			\
	-DBURNER_SYSCONFDIR=\"$(sysconfdir)\"   			\
	-DBURNER_DATADIR=\"$(datadir)/burner\"     	    		\
	-DBURNER_LIBDIR=\"$(libdir)\"  	         		\
	$(WARN_CFLAGS)							\
	$(DISABLE_DEPRECATED)						\
	$(BURNER_GLIB_CFLAGS)						\
	$(BURNER_GIO_CFLAGS)						\
	$(BURNER_PL_PARSER_CFLAGS)					\
	$(BURNER_GSTREAMER_CFLAGS)					\
	$(BURNER_GSTREAMER_BASE_CFLAGS)				\
	$(BURNER_GTK_CFLAGS)


lib_LTLIBRARIES = \
	libburner-utils3.la

libburner_utils3_la_LIBADD =					\
	$(BURNER_GLIB_LIBS)					\
	$(BURNER_GIO_LIBS)		\
	$(BURNER_GSTREAMER_LIBS)	\
	$(BURNER_GSTREAMER_BASE_LIBS)	\
	$(BURNER_PL_PARSER_LIBS)	\
	$(BURNER_GTK_LIBS)

libburner_utils3_la_LDFLAGS =					\
	-version-info $(LIBBURNER_LT_VERSION)			\
	-no-undefined
#	-export-symbols ./libburner-utils.symbols

libburner_utils3_la_SOURCES =		\
	burner-disc-message.c        \
	burner-disc-message.h        \
	burner-misc.c        \
	burner-misc.h        \
	burner-notify.c        \
	burner-notify.h        \
	burner-jacket-background.c        \
	burner-jacket-background.h        \
	burner-jacket-buffer.c        \
	burner-jacket-buffer.h        \
	burner-jacket-edit.c        \
	burner-jacket-edit.h        \
	burner-jacket-font.c        \
	burner-jacket-font.h        \
	burner-jacket-view.c        \
	burner-jacket-view.h        \
	burner-tool-color-picker.c        \
	burner-tool-color-picker.h        \
	burner-async-task-manager.c        \
	burner-async-task-manager.h        \
	burner-io.c        \
	burner-io.h        \
	burner-metadata.c        \
	burner-metadata.h        \
	burner-pk.c        \
	burner-pk.h

# EXTRA_DIST =			\
#	libburner-utils.symbols

CLEANFILES =			\
	$(RECMARSHALFILES)

-include $(top_srcdir)/git.mk
