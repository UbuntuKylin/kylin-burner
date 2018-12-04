
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
	$(BURNER_GMODULE_EXPORT_CFLAGS)					\
	$(BURNER_GIO_CFLAGS)						\
	$(BURNER_GTK_CFLAGS)

GLIB_GENMARSHAL=`pkg-config --variable=glib_genmarshal glib-2.0`
RECMARSHALFILES = libburner-marshal.h libburner-marshal.c

libburner-marshal.h: libburner-marshal.list
	( $(GLIB_GENMARSHAL) --prefix=burner_marshal $(srcdir)/libburner-marshal.list --header > libburner-marshal.h )
libburner-marshal.c: libburner-marshal.h
	( $(GLIB_GENMARSHAL) --prefix=burner_marshal $(srcdir)/libburner-marshal.list --body --header > libburner-marshal.c )

headerdir=$(includedir)/burner3
header_HEADERS =	burner-drive.h			\
		burner-medium-monitor.h		\
		burner-media.h			\
		burner-units.h			\
		burner-medium.h			\
		burner-volume.h		\
		burner-medium-selection.h	\
		burner-drive-selection.h

lib_LTLIBRARIES = 				\
	libburner-media3.la

libburner_media3_la_LIBADD =                                     \
	$(BURNER_GLIB_LIBS)                                     \
	$(BURNER_GMODULE_EXPORT_LIBS)                                     \
	$(BURNER_GTHREAD_LIBS)                                     \
	$(BURNER_GIO_LIBS)                                     \
	$(BURNER_GTK_LIBS)                                     \
	$(BURNER_SCSI_LIBS)

libburner_media3_la_LDFLAGS =					\
	-version-info $(LIBBURNER_LT_VERSION)			\
	-no-undefined
	$(AM_LDFLAGS)
#	-export-symbols ./libburner-media.symbols

libburner_media3_la_SOURCES =		\
	libburner-marshal.c		\
	libburner-marshal.h		\
	burner-drive.h			\
	burner-medium-monitor.h		\
	burner-media.h			\
	burner-units.h			\
	burner-medium.h			\
	burner-volume.h		\
	burner-medium-selection.h	\
	scsi-base.h         \
	scsi-command.h         \
	scsi-error.h         \
	scsi-get-configuration.c         \
	scsi-get-configuration.h         \
	scsi-mmc1.h         \
	scsi-mmc2.h         \
	scsi-opcodes.h         \
	scsi-read-disc-info.c         \
	scsi-read-disc-info.h         \
	scsi-read-toc-pma-atip.c         \
	scsi-read-toc-pma-atip.h         \
	scsi-sense-data.c         \
	scsi-sense-data.h         \
	scsi-utils.h         \
	scsi-q-subchannel.h         \
	scsi-error.c         \
	scsi-read-track-information.c         \
	scsi-read-track-information.h         \
	scsi-get-performance.c         \
	scsi-get-performance.h         \
	scsi-mmc3.h         \
	scsi-mode-pages.h         \
	scsi-mode-sense.c         \
	scsi-status-page.h         \
	scsi-spc1.h         \
	scsi-read-capacity.c         \
	scsi-read-capacity.h         \
	scsi-read-disc-structure.c         \
	scsi-read-disc-structure.h         \
	scsi-dvd-structures.h         \
	scsi-read-format-capacities.c         \
	scsi-read-format-capacities.h         \
	scsi-read-cd.h	\
	scsi-read-cd.c	\
	scsi-device.h         \
	scsi-mech-status.c         \
	scsi-mech-status.h         \
	scsi-write-page.h         \
	scsi-mode-select.c         \
	scsi-read10.c         \
	scsi-sbc.h		\
	scsi-test-unit-ready.c           \
	burner-media.c           \
	burner-medium-monitor.c         \
	burn-susp.c         \
	burn-susp.h         \
	burn-iso-field.c         \
	burn-iso-field.h         \
	burn-iso9660.c         \
	burn-iso9660.h         \
	burn-volume-source.c         \
	burn-volume-source.h         \
	burn-volume.c         \
	burn-volume.h         \
	burner-medium.c         \
	burner-volume.c         \
	burner-drive.c         \
	burner-medium-selection.c         \
	burner-units.c         \
	burner-drive-selection.c         \
	burner-drive-selection.h         \
	burner-media-private.h         \
	burner-medium-selection-priv.h         \
	burner-gio-operation.h         \
	burner-gio-operation.c         \
	scsi-inquiry.c         \
	scsi-prevent-allow-medium-removal.c         \
	scsi-inquiry.h         \
	burner-drive-priv.h

# FreeBSD's SCSI CAM interface
if HAVE_CAM_LIB_H
libburner_media3_la_SOURCES += scsi-cam.c
endif

# Linux's SCSI CAM interface
if HAVE_SG_IO_HDR_T
libburner_media3_la_SOURCES += scsi-sg.c
endif

# NetBSD's scsi(4)
if HAVE_SCSIIO_H
libburner_media3_la_SOURCES += scsi-netbsd.c
endif

# Solaris's USCSI interface
if HAVE_USCSI_H
libburner_media3_la_SOURCES += scsi-uscsi.c
endif

if HAVE_INTROSPECTION
girdir = $(INTROSPECTION_GIRDIR)
gir_DATA = BurnerMedia-@TYPELIB_VERSION@.gir

typelibsdir = $(INTROSPECTION_TYPELIBDIR)
typelibs_DATA = BurnerMedia-@TYPELIB_VERSION@.typelib

BurnerMedia-@TYPELIB_VERSION@.gir: libburner-media3.la $(INTROSPECTION_SCANNER)
	$(INTROSPECTION_SCANNER) -v --namespace=BurnerMedia --nsversion=3.1 \
            --include=Gtk-3.0 \
	    -I$(top_srcdir)/libburner-media	\
	    --library=burner-media3 \
	    --libtool="$(LIBTOOL)" \
            --output $@ \
            --pkg=gtk+-3.0  \
	    --strip-prefix=Burner	\
            $(top_srcdir)/libburner-media/burner-drive.h			\
	    $(top_srcdir)/libburner-media/burner-medium-monitor.h		\
	    $(top_builddir)/libburner-media/burner-media.h			\
	    $(top_srcdir)/libburner-media/burner-units.h			\
	    $(top_srcdir)/libburner-media/burner-medium.h			\
	    $(top_srcdir)/libburner-media/burner-volume.h		\
	    $(top_srcdir)/libburner-media/burner-medium-selection.h	\
	    $(top_srcdir)/libburner-media/burner-drive-selection.h	\
            $(top_srcdir)/libburner-media/*.c	    

BurnerMedia-@TYPELIB_VERSION@.typelib: $(gir_DATA) $(INTROSPECTION_COMPILER)
	$(INTROSPECTION_COMPILER) $(top_builddir)/libburner-media/$(gir_DATA) --shared-library=libburner-media3 -o $(typelibs_DATA)
endif

EXTRA_DIST =			\
	libburner-marshal.list
#	libburner-media.symbols

CLEANFILES =			\
	$(RECMARSHALFILES)  \
	$(gir_DATA)         \
	$(typelibs_DATA)

-include $(top_srcdir)/git.mk
