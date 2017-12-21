/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Libburner-burn
 * Copyright (C) Philippe Rouquier 2005-2009 <bonfire-app@wanadoo.fr>
 *
 * Libburner-burn is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * The Libburner-burn authors hereby grant permission for non-GPL compatible
 * GStreamer plugins to be used and distributed together with GStreamer
 * and Libburner-burn. This permission is above and beyond the permissions granted
 * by the GPL license by which Libburner-burn is covered. If you modify this code
 * you may extend this exception to your version of the code, but you are not
 * obligated to do so. If you do not wish to do so, delete this exception
 * statement from your version.
 * 
 * Libburner-burn is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#ifndef _BURNER_TRACK_DATA_H_
#define _BURNER_TRACK_DATA_H_

#include <glib-object.h>

#include <burner-track.h>

G_BEGIN_DECLS

/**
 * BurnerGraftPt:
 * @uri: a URI
 * @path: a file path
 *
 * A pair of strings describing:
 * @uri the actual current location of the file
 * @path the path of the file on the future ISO9660/UDF/... filesystem
 **/
typedef struct _BurnerGraftPt {
	gchar *uri;
	gchar *path;
} BurnerGraftPt;

void
burner_graft_point_free (BurnerGraftPt *graft);

BurnerGraftPt *
burner_graft_point_copy (BurnerGraftPt *graft);


#define BURNER_TYPE_TRACK_DATA             (burner_track_data_get_type ())
#define BURNER_TRACK_DATA(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BURNER_TYPE_TRACK_DATA, BurnerTrackData))
#define BURNER_TRACK_DATA_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BURNER_TYPE_TRACK_DATA, BurnerTrackDataClass))
#define BURNER_IS_TRACK_DATA(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BURNER_TYPE_TRACK_DATA))
#define BURNER_IS_TRACK_DATA_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BURNER_TYPE_TRACK_DATA))
#define BURNER_TRACK_DATA_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BURNER_TYPE_TRACK_DATA, BurnerTrackDataClass))

typedef struct _BurnerTrackDataClass BurnerTrackDataClass;
typedef struct _BurnerTrackData BurnerTrackData;

struct _BurnerTrackDataClass
{
	BurnerTrackClass parent_class;

	/* virtual functions */

	/**
	 * set_source:
	 * @track: a #BurnerTrackData.
	 * @grafts: (element-type BurnerBurn.GraftPt) (transfer full): a #GSList of #BurnerGraftPt.
	 * @unreadable: (element-type utf8) (transfer full) (allow-none): a #GSList of URIs (as strings) or %NULL.
	 *
	 * Return value: a #BurnerBurnResult
	 **/
	BurnerBurnResult	(*set_source)		(BurnerTrackData *track,
							 GSList *grafts,
							 GSList *unreadable);
	BurnerBurnResult       (*add_fs)		(BurnerTrackData *track,
							 BurnerImageFS fstype);

	BurnerBurnResult       (*rm_fs)		(BurnerTrackData *track,
							 BurnerImageFS fstype);

	BurnerImageFS		(*get_fs)		(BurnerTrackData *track);
	GSList*			(*get_grafts)		(BurnerTrackData *track);
	GSList*			(*get_excluded)		(BurnerTrackData *track);
	guint64			(*get_file_num)		(BurnerTrackData *track);
};

struct _BurnerTrackData
{
	BurnerTrack parent_instance;
};

GType burner_track_data_get_type (void) G_GNUC_CONST;

BurnerTrackData *
burner_track_data_new (void);

BurnerBurnResult
burner_track_data_set_source (BurnerTrackData *track,
			       GSList *grafts,
			       GSList *unreadable);

BurnerBurnResult
burner_track_data_add_fs (BurnerTrackData *track,
			   BurnerImageFS fstype);

BurnerBurnResult
burner_track_data_rm_fs (BurnerTrackData *track,
			  BurnerImageFS fstype);

BurnerBurnResult
burner_track_data_set_data_blocks (BurnerTrackData *track,
				    goffset blocks);

BurnerBurnResult
burner_track_data_set_file_num (BurnerTrackData *track,
				 guint64 number);

GSList *
burner_track_data_get_grafts (BurnerTrackData *track);

GSList *
burner_track_data_get_excluded_list (BurnerTrackData *track);

BurnerBurnResult
burner_track_data_write_to_paths (BurnerTrackData *track,
                                   const gchar *grafts_path,
                                   const gchar *excluded_path,
                                   const gchar *emptydir,
                                   const gchar *videodir,
                                   GError **error);

BurnerBurnResult
burner_track_data_get_file_num (BurnerTrackData *track,
				 guint64 *file_num);

BurnerImageFS
burner_track_data_get_fs (BurnerTrackData *track);

G_END_DECLS

#endif /* _BURNER_TRACK_DATA_H_ */
