
AM_CPPFLAGS = \
	-I$(top_srcdir)							\
	-I$(top_srcdir)/libburner-utils/				\
	-I$(top_builddir)/libburner-utils/				\
	-I$(top_srcdir)/libburner-media/				\
	-I$(top_builddir)/libburner-media/				\
	-I$(top_srcdir)/libburner-burn/				\
	-I$(top_builddir)/libburner-burn/				\
	-DBURNER_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" 	\
	-DBURNER_PREFIX=\"$(prefix)\"           			\
	-DBURNER_SYSCONFDIR=\"$(sysconfdir)\"   			\
	-DBURNER_DATADIR=\"$(datadir)/burner\"     	    		\
	-DBURNER_LIBDIR=\"$(libdir)\"  	         		\
	-DEGG_SM_CLIENT_BACKEND_XSMP					\
	$(WARN_CFLAGS)							\
	$(DISABLE_DEPRECATED)						\
	$(BURNER_GLIB_CFLAGS)						\
	$(BURNER_GIO_CFLAGS)						\
	$(BURNER_GSTREAMER_CFLAGS)					\
	$(BURNER_GSTREAMER_BASE_CFLAGS)				\
	$(BURNER_GTK_CFLAGS)						\
	$(BURNER_DBUS_CFLAGS)						\
	$(BURNER_LIBXML_CFLAGS)					\
	$(BURNER_SEARCH_CFLAGS)					\
	$(BURNER_PL_PARSER_CFLAGS)					\
	$(BURNER_SM_CFLAGS)							\
	-Wno-error=format-y2k

GLIB_GENMARSHAL=`pkg-config --variable=glib_genmarshal glib-2.0`
RECMARSHALFILES = burner-marshal.h burner-marshal.c

burner-marshal.h: burner-marshal.list
	( $(GLIB_GENMARSHAL) --prefix=burner_marshal $(srcdir)/burner-marshal.list --header > burner-marshal.h )
burner-marshal.c: burner-marshal.h
	( $(GLIB_GENMARSHAL) --prefix=burner_marshal $(srcdir)/burner-marshal.list --body --header > burner-marshal.c )

bin_PROGRAMS = burner

burner_SOURCES = \
	burner-marshal.c	\
	burner-marshal.h	\
	main.c		\
	burner-utils.h		\
	burner-utils.c         \
	burner-player.c         \
	burner-player.h         \
	burner-eject-dialog.c		\
	burner-eject-dialog.h		\
	burner-disc.c         \
	burner-disc.h         \
	burner-audio-disc.c         \
	burner-audio-disc.h         \
	burner-project.c         \
	burner-project.h         \
	burner-project-type-chooser.c         \
	burner-project-type-chooser.h         \
	burner-player-bacon.c         \
	burner-player-bacon.h         \
	burner-song-properties.c         \
	burner-song-properties.h         \
	burner-uri-container.c         \
	burner-uri-container.h         \
	burner-project-manager.c         \
	burner-project-manager.h         \
	burner-layout.c         \
	burner-layout.h         \
	burner-file-chooser.c         \
	burner-file-chooser.h         \
	eggtreemultidnd.c         \
	eggtreemultidnd.h         \
	burner-multi-dnd.c         \
	burner-multi-dnd.h         \
	burner-layout-object.c         \
	burner-layout-object.h         \
	burner-plugin-manager-ui.c         \
	burner-plugin-manager-ui.h         \
	burner-pref.h         \
	burner-pref.c         \
	burner-plugin-option.h         \
	burner-plugin-option.c         \
	burner-split-dialog.h         \
	burner-split-dialog.c         \
	burner-time-button.h         \
	burner-time-button.c         \
	burner-preview.h         \
	burner-preview.c         \
	burner-data-disc.h         \
	burner-data-disc.c         \
	burner-file-filtered.h         \
	burner-file-filtered.c         \
	burner-filter-option.h         \
	burner-filter-option.c         \
	baobab-cell-renderer-progress.c         \
	baobab-cell-renderer-progress.h         \
	burner-multi-song-props.h         \
	burner-multi-song-props.c         \
	burner-rename.h         \
	burner-rename.c         \
	burner-video-disc.c         \
	burner-video-disc.h         \
	burner-video-tree-model.c         \
	burner-video-tree-model.h         \
	burner-app.c		\
	burner-app.h           \
	burner-project-name.c           \
	burner-project-name.h           \
	burner-project-parse.c           \
	burner-project-parse.h           \
	burner-setting.h        \
	burner-setting.c        \
	burner-search-engine.h        \
	burner-search-engine.c        \
	burner-drive-settings.h        \
	burner-drive-settings.c	\
	burner-song-control.h        \
	burner-song-control.c        \
	burner-cli.c        \
	burner-cli.h

if BUILD_PLAYLIST
burner_SOURCES += \
	burner-playlist.c         \
	burner-playlist.h

endif

if BUILD_TRACKER
burner_SOURCES += \
		   burner-search-tracker.h	\
		   burner-search-tracker.c
endif

burner_LDADD =						\
	$(top_builddir)/libburner-media/libburner-media3.la	\
	$(top_builddir)/libburner-burn/libburner-burn3.la	\
	$(top_builddir)/libburner-utils/libburner-utils3.la	\
	$(BURNER_GLIB_LIBS)		\
	$(BURNER_GTHREAD_LIBS)				\
	$(BURNER_GIO_LIBS)		\
	$(BURNER_GSTREAMER_LIBS)	\
	$(BURNER_GSTREAMER_BASE_LIBS)	\
	$(BURNER_GTK_LIBS)		\
	$(BURNER_GMODULE_LIBS)		\
	$(BURNER_DBUS_LIBS)		\
	$(BURNER_LIBXML_LIBS)		\
	$(BURNER_SEARCH_LIBS)		\
	$(BURNER_PL_PARSER_LIBS)	\
	$(BURNER_SM_LIBS)

EXTRA_DIST =			\
	burner-marshal.list

CLEANFILES =			\
	$(RECMARSHALFILES)

-include $(top_srcdir)/git.mk
