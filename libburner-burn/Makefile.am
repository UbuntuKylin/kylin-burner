AM_CPPFLAGS = \
	-I$(top_srcdir)							\
	-I$(top_builddir)						\
	-I$(top_srcdir)/libburner-utils/				\
	-I$(top_builddir)/libburner-utils/				\
	-I$(top_srcdir)/libburner-media/				\
	-I$(top_builddir)/libburner-media/				\
	-DBURNER_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" 	\
	-DBURNER_PREFIX=\"$(prefix)\"           			\
	-DBURNER_SYSCONFDIR=\"$(sysconfdir)\"   			\
	-DBURNER_DATADIR=\"$(datadir)/burner\"     	    		\
	-DBURNER_LIBDIR=\"$(libdir)\"  	         		\
	-DBURNER_PLUGIN_DIRECTORY=\"$(BURNER_PLUGIN_DIRECTORY)\"	\
	$(WARN_CFLAGS)							\
	$(DISABLE_DEPRECATED)						\
	$(BURNER_GMODULE_CFLAGS)					\
	$(BURNER_GLIB_CFLAGS)						\
	$(BURNER_GIO_CFLAGS)						\
	$(BURNER_GTK_CFLAGS)						\
	$(BURNER_LIBNOTIFY_CFLAGS) \
	$(BURNER_GSTREAMER_CFLAGS)					\
	$(BURNER_CANBERRA_CFLAGS)					\
	-Wno-error=format-nonliteral

GLIB_GENMARSHAL=`pkg-config --variable=glib_genmarshal glib-2.0`
RECMARSHALFILES = libburner-marshal.h libburner-marshal.c

libburner-marshal.h: libburner-marshal.list
	( $(GLIB_GENMARSHAL) --prefix=burner_marshal $(srcdir)/libburner-marshal.list --header > libburner-marshal.h )
libburner-marshal.c: libburner-marshal.h
	( $(GLIB_GENMARSHAL) --prefix=burner_marshal $(srcdir)/libburner-marshal.list --body --header > libburner-marshal.c )

headerdir=$(includedir)/burner3
header_HEADERS =	burner-tags.h						\
		burner-enums.h						\
		burner-error.h						\
		burner-status.h					\
		burner-burn-lib.h					\
		burner-track-type.h					\
		burner-track.h						\
		burner-track-data.h                 			\
		burner-track-data-cfg.h				\
		burner-track-disc.h                 			\
		burner-track-stream.h                 			\
		burner-track-stream-cfg.h             			\
		burner-track-image.h					\
		burner-track-image-cfg.h				\
		burner-session.h                 			\
		burner-session-span.h					\
		burner-session-cfg.h					\
		burner-burn.h						\
		burner-burn-options.h        			        \
		burner-burn-dialog.h					\
		burner-tool-dialog.h					\
		burner-sum-dialog.h					\
		burner-blank-dialog.h

lib_LTLIBRARIES = \
	libburner-burn3.la

libburner_burn3_la_LIBADD =					\
	../libburner-media/libburner-media3.la			\
	../libburner-utils/libburner-utils3.la			\
	$(BURNER_GLIB_LIBS)					\
	$(BURNER_GTHREAD_LIBS)					\
	$(BURNER_GMODULE_LIBS)					\
	$(BURNER_GIO_LIBS)					\
	$(BURNER_GTK_LIBS)					\
	$(BURNER_GSTREAMER_LIBS)				\
	$(BURNER_LIBNOTIFY_LIBS)					\
	$(BURNER_CANBERRA_LIBS)				\
	$(LIBM)

libburner_burn3_la_LDFLAGS =					\
	-version-info $(LIBBURNER_LT_VERSION)			\
	-no-undefined						\
	-export-dynamic
#	-export-symbols ./libburner-burn.symbols

libburner_burn3_la_SOURCES = 		\
	libburner-marshal.c		\
	libburner-marshal.h		\
	burner-tool-dialog.c         \
	burner-tool-dialog.h         \
	burner-sum-dialog.c         \
	burner-sum-dialog.h         \
	burner-blank-dialog.c         \
	burner-blank-dialog.h         \
	burner-burn.c			\
	burner-burn.h			\
	burner-xfer.c			\
	burner-xfer.h			\
	burn-basics.h                 \
	burn-caps.h                 \
	burn-dbus.h                 \
	burn-debug.h                 \
	burn-image-format.h                 \
	burn-job.h                 \
	burn-mkisofs-base.h                 \
	burn-plugin-manager.h                 \
	burn-process.h                 \
	burner-session.h                 \
	burn-task.h                 \
	burn-task-ctx.h                 \
	burn-task-item.h                 \
	burner-track.h                 \
	burner-session.c                 \
	burner-track.c                 \
	burn-basics.c                 \
	burn-caps.c                 \
	burn-dbus.c                 \
	burn-debug.c                 \
	burn-image-format.c                 \
	burn-job.c                 \
	burn-mkisofs-base.c                 \
	burn-plugin.c                 \
	burn-plugin-manager.c                 \
	burn-process.c                 \
	burn-task.c                 \
	burn-task-ctx.c                 \
	burn-task-item.c                 \
	burner-burn-dialog.c                 \
	burner-burn-dialog.h                 \
	burner-burn-options.c                 \
	burner-burn-options.h                 \
	burner-dest-selection.c                 \
	burner-dest-selection.h                 \
	burner-drive-properties.c                 \
	burner-drive-properties.h                 \
	burner-image-properties.c                 \
	burner-image-properties.h                 \
	burner-image-type-chooser.c                 \
	burner-image-type-chooser.h                 \
	burner-medium-properties.c                 \
	burner-medium-properties.h                 \
	burner-progress.c                 \
	burner-progress.h                 \
	burner-session-cfg.c                 \
	burner-session-cfg.h                 \
	burner-src-image.c                 \
	burner-src-image.h                 \
	burner-src-selection.c                 \
	burner-src-selection.h                 \
	burner-burn-lib.h                 \
	burner-error.h                 \
	burner-enums.h                 \
	burner-tags.h                 \
	burner-cover.c                 \
	burner-cover.h                 \
	burner-track-disc.c                 \
	burner-track-disc.h                 \
	burner-track-stream.c                 \
	burner-track-stream.h                 \
	burner-track-image.c                 \
	burner-track-image.h                 \
	burner-track-data.c                 \
	burner-track-data.h                 \
	burner-track-image-cfg.c                 \
	burner-track-image-cfg.h                 \
	burner-caps-plugin.c                 \
	burner-caps-burn.c                 \
	burner-caps-session.c                 \
	burner-caps-burn.h                 \
	burner-plugin.h                 \
	burner-plugin-information.h                 \
	burner-plugin-registration.h                 \
	burner-track-type.c                 \
	burner-track-type.h                 \
	burner-track-type-private.h                 \
	burner-status.c                 \
	burner-status.h                 \
	burner-status-dialog.c                 \
	burner-status-dialog.h                 \
	burner-session-helper.h                 \
	burner-data-project.c                 \
	burner-data-project.h                 \
	burner-data-session.c                 \
	burner-data-session.h                 \
	burner-data-vfs.c                 \
	burner-data-vfs.h                 \
	burner-file-node.c                 \
	burner-file-node.h                 \
	burner-data-tree-model.c                 \
	burner-data-tree-model.h                 \
	burner-track-data-cfg.c                 \
	burner-track-data-cfg.h                 \
	burner-filtered-uri.c                 \
	burner-filtered-uri.h                 \
	burner-track-stream-cfg.c                 \
	burner-track-stream-cfg.h                 \
	burner-tool-dialog-private.h                 \
	burner-video-options.h                 \
	burner-video-options.c                 \
	burner-customize-title.h       \
	burner-customize-title.c       \
	burner-session-span.h                 \
	burner-session-span.c                 \
	burner-plugin-private.h

if BUILD_INOTIFY
libburner_burn3_la_SOURCES += burner-file-monitor.c burner-file-monitor.h
endif

if HAVE_APP_INDICATOR
libburner_burn3_la_SOURCES += burner-app-indicator.h burner-app-indicator.c
libburner_burn3_la_LIBADD += @APP_INDICATOR_LIBS@
AM_CPPFLAGS += -DHAVE_APP_INDICATOR @APP_INDICATOR_CFLAGS@
endif

if HAVE_UNITY
libburner_burn3_la_LIBADD += @UNITY_LIBS@
AM_CPPFLAGS += -DHAVE_UNITY @UNITY_CFLAGS@
endif

EXTRA_DIST =			\
	libburner-marshal.list
#	libburner-burn.symbols

if HAVE_INTROSPECTION
girdir = $(INTROSPECTION_GIRDIR)
gir_DATA = BurnerBurn-@TYPELIB_VERSION@.gir

typelibsdir = $(INTROSPECTION_TYPELIBDIR)
typelibs_DATA = BurnerBurn-@TYPELIB_VERSION@.typelib

BurnerBurn-@TYPELIB_VERSION@.gir: libburner-burn3.la $(INTROSPECTION_SCANNER)
	$(INTROSPECTION_SCANNER) -v --namespace=BurnerBurn --nsversion=3.1 \
	    -I$(top_srcdir)/libburner-media	\
	    -I$(top_builddir)/libburner-media	\
	    -I$(top_srcdir)/libburner-burn		\
	    -I$(top_builddir)/libburner-burn	\
            --include=Gtk-3.0 \
            --add-include-path=$(top_builddir)/libburner-media	\
	    --include=BurnerMedia-@TYPELIB_VERSION@ \
	    --library=burner-burn3 \
	    --libtool="$(LIBTOOL)" \
            --output $@ \
            --pkg=gtk+-3.0  \
	    --strip-prefix=Burner	\
            $(top_srcdir)/libburner-burn/burner-tags.h						\
            $(top_srcdir)/libburner-burn/burner-enums.h						\
            $(top_srcdir)/libburner-burn/burner-error.h						\
            $(top_srcdir)/libburner-burn/burner-status.h					\
            $(top_builddir)/libburner-burn/burner-burn-lib.h					\
            $(top_srcdir)/libburner-burn/burner-track-type.h					\
            $(top_srcdir)/libburner-burn/burner-track.h						\
            $(top_srcdir)/libburner-burn/burner-track-data.h                 			\
            $(top_srcdir)/libburner-burn/burner-track-data-cfg.h				\
            $(top_srcdir)/libburner-burn/burner-track-disc.h                 			\
            $(top_srcdir)/libburner-burn/burner-track-stream.h                 			\
            $(top_srcdir)/libburner-burn/burner-track-stream-cfg.h             			\
            $(top_srcdir)/libburner-burn/burner-track-image.h					\
            $(top_srcdir)/libburner-burn/burner-track-image-cfg.h				\
            $(top_srcdir)/libburner-burn/burner-session.h                 			\
            $(top_srcdir)/libburner-burn/burner-session-span.h					\
            $(top_srcdir)/libburner-burn/burner-session-cfg.h					\
            $(top_srcdir)/libburner-burn/burner-burn.h						\
            $(top_srcdir)/libburner-burn/burner-burn-options.h        			        \
            $(top_srcdir)/libburner-burn/burner-burn-dialog.h					\
            $(top_srcdir)/libburner-burn/burner-tool-dialog.h					\
            $(top_srcdir)/libburner-burn/burner-sum-dialog.h					\
            $(top_srcdir)/libburner-burn/*.c

BurnerBurn-@TYPELIB_VERSION@.typelib: $(gir_DATA) $(INTROSPECTION_COMPILER)
	$(INTROSPECTION_COMPILER) $(top_builddir)/libburner-burn/$(gir_DATA) --shared-library=libburner-burn3 -o $(typelibs_DATA) --includedir=$(top_builddir)/libburner-media
endif

CLEANFILES =			\
	$(RECMARSHALFILES)  \
	$(gir_DATA)         \
	$(typelibs_DATA)


-include $(top_srcdir)/git.mk
