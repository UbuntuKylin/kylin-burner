/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Libburner-media
 * Copyright (C) Philippe Rouquier 2005-2009 <bonfire-app@wanadoo.fr>
 *
 * Libburner-media is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * The Libburner-media authors hereby grant permission for non-GPL compatible
 * GStreamer plugins to be used and distributed together with GStreamer
 * and Libburner-media. This permission is above and beyond the permissions granted
 * by the GPL license by which Libburner-media is covered. If you modify this code
 * you may extend this exception to your version of the code, but you are not
 * obligated to do so. If you do not wish to do so, delete this exception
 * statement from your version.
 * 
 * Libburner-media is distributed in the hope that it will be useful,
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

#include <glib.h>

#include "scsi-base.h"

#ifndef _SCSI_GET_CONFIGURATION_H
#define _SCSI_GET_CONFIGURATION_H

G_BEGIN_DECLS

typedef enum {
BURNER_SCSI_PROF_EMPTY				= 0x0000,
BURNER_SCSI_PROF_NON_REMOVABLE		= 0x0001,
BURNER_SCSI_PROF_REMOVABLE		= 0x0002,
BURNER_SCSI_PROF_MO_ERASABLE		= 0x0003,
BURNER_SCSI_PROF_MO_WRITE_ONCE		= 0x0004,
BURNER_SCSI_PROF_MO_ADVANCED_STORAGE	= 0x0005,
	/* reserved */
BURNER_SCSI_PROF_CDROM			= 0x0008,
BURNER_SCSI_PROF_CDR			= 0x0009,
BURNER_SCSI_PROF_CDRW			= 0x000A,
	/* reserved */
BURNER_SCSI_PROF_DVD_ROM		= 0x0010,
BURNER_SCSI_PROF_DVD_R			= 0x0011,
BURNER_SCSI_PROF_DVD_RAM		= 0x0012,
BURNER_SCSI_PROF_DVD_RW_RESTRICTED	= 0x0013,
BURNER_SCSI_PROF_DVD_RW_SEQUENTIAL	= 0x0014,
BURNER_SCSI_PROF_DVD_R_DL_SEQUENTIAL	= 0x0015,
BURNER_SCSI_PROF_DVD_R_DL_JUMP		= 0x0016,
	/* reserved */
BURNER_SCSI_PROF_DVD_RW_PLUS		= 0x001A,
BURNER_SCSI_PROF_DVD_R_PLUS		= 0x001B,
	/* reserved */
BURNER_SCSI_PROF_DDCD_ROM		= 0x0020,
BURNER_SCSI_PROF_DDCD_R		= 0x0021,
BURNER_SCSI_PROF_DDCD_RW		= 0x0022,
	/* reserved */
BURNER_SCSI_PROF_DVD_RW_PLUS_DL	= 0x002A,
BURNER_SCSI_PROF_DVD_R_PLUS_DL		= 0x002B,
	/* reserved */
BURNER_SCSI_PROF_BD_ROM		= 0x0040,
BURNER_SCSI_PROF_BR_R_SEQUENTIAL	= 0x0041,
BURNER_SCSI_PROF_BR_R_RANDOM		= 0x0042,
BURNER_SCSI_PROF_BD_RW			= 0x0043,
BURNER_SCSI_PROF_HD_DVD_ROM		= 0x0050,
BURNER_SCSI_PROF_HD_DVD_R		= 0x0051,
BURNER_SCSI_PROF_HD_DVD_RAM		= 0x0052,
	/* reserved */
} BurnerScsiProfile;

typedef enum {
BURNER_SCSI_INTERFACE_NONE		= 0x00000000,
BURNER_SCSI_INTERFACE_SCSI		= 0x00000001,
BURNER_SCSI_INTERFACE_ATAPI		= 0x00000002,
BURNER_SCSI_INTERFACE_FIREWIRE_95	= 0x00000003,
BURNER_SCSI_INTERFACE_FIREWIRE_A	= 0x00000004,
BURNER_SCSI_INTERFACE_FCP		= 0x00000005,
BURNER_SCSI_INTERFACE_FIREWIRE_B	= 0x00000006,
BURNER_SCSI_INTERFACE_SERIAL_ATAPI	= 0x00000007,
BURNER_SCSI_INTERFACE_USB		= 0x00000008
} BurnerScsiInterface;

typedef enum {
BURNER_SCSI_LOADING_CADDY		= 0x000,
BURNER_SCSI_LOADING_TRAY		= 0x001,
BURNER_SCSI_LOADING_POPUP		= 0x002,
BURNER_SCSI_LOADING_EMBED_CHANGER_IND	= 0X004,
BURNER_SCSI_LOADING_EMBED_CHANGER_MAG	= 0x005
} BurnerScsiLoadingMech;

typedef enum {
BURNER_SCSI_FEAT_PROFILES		= 0x0000,
BURNER_SCSI_FEAT_CORE			= 0x0001,
BURNER_SCSI_FEAT_MORPHING		= 0x0002,
BURNER_SCSI_FEAT_REMOVABLE		= 0x0003,
BURNER_SCSI_FEAT_WRT_PROTECT		= 0x0004,
	/* reserved */
BURNER_SCSI_FEAT_RD_RANDOM		= 0x0010,
	/* reserved */
BURNER_SCSI_FEAT_RD_MULTI		= 0x001D,
BURNER_SCSI_FEAT_RD_CD			= 0x001E,
BURNER_SCSI_FEAT_RD_DVD		= 0x001F,
BURNER_SCSI_FEAT_WRT_RANDOM		= 0x0020,
BURNER_SCSI_FEAT_WRT_INCREMENT		= 0x0021,
BURNER_SCSI_FEAT_WRT_ERASE		= 0x0022,
BURNER_SCSI_FEAT_WRT_FORMAT		= 0x0023,
BURNER_SCSI_FEAT_DEFECT_MNGT		= 0x0024,
BURNER_SCSI_FEAT_WRT_ONCE		= 0x0025,
BURNER_SCSI_FEAT_RESTRICT_OVERWRT	= 0x0026,
BURNER_SCSI_FEAT_WRT_CAV_CDRW		= 0x0027,
BURNER_SCSI_FEAT_MRW			= 0x0028,
BURNER_SCSI_FEAT_DEFECT_REPORT		= 0x0029,
BURNER_SCSI_FEAT_WRT_DVDRW_PLUS	= 0x002A,
BURNER_SCSI_FEAT_WRT_DVDR_PLUS		= 0x002B,
BURNER_SCSI_FEAT_RIGID_OVERWRT		= 0x002C,
BURNER_SCSI_FEAT_WRT_TAO		= 0x002D,
BURNER_SCSI_FEAT_WRT_SAO_RAW		= 0x002E,
BURNER_SCSI_FEAT_WRT_DVD_LESS		= 0x002F,
BURNER_SCSI_FEAT_RD_DDCD		= 0x0030,
BURNER_SCSI_FEAT_WRT_DDCD		= 0x0031,
BURNER_SCSI_FEAT_RW_DDCD		= 0x0032,
BURNER_SCSI_FEAT_LAYER_JUMP		= 0x0033,
BURNER_SCSI_FEAT_WRT_CDRW		= 0x0037,
BURNER_SCSI_FEAT_BDR_POW		= 0x0038,
	/* reserved */
BURNER_SCSI_FEAT_WRT_DVDRW_PLUS_DL		= 0x003A,
BURNER_SCSI_FEAT_WRT_DVDR_PLUS_DL		= 0x003B,
	/* reserved */
BURNER_SCSI_FEAT_RD_BD			= 0x0040,
BURNER_SCSI_FEAT_WRT_BD		= 0x0041,
BURNER_SCSI_FEAT_TSR			= 0x0042,
	/* reserved */
BURNER_SCSI_FEAT_RD_HDDVD		= 0x0050,
BURNER_SCSI_FEAT_WRT_HDDVD		= 0x0051,
	/* reserved */
BURNER_SCSI_FEAT_HYBRID_DISC		= 0x0080,
	/* reserved */
BURNER_SCSI_FEAT_PWR_MNGT		= 0x0100,
BURNER_SCSI_FEAT_SMART			= 0x0101,
BURNER_SCSI_FEAT_EMBED_CHNGR		= 0x0102,
BURNER_SCSI_FEAT_AUDIO_PLAY		= 0x0103,
BURNER_SCSI_FEAT_FIRM_UPGRADE		= 0x0104,
BURNER_SCSI_FEAT_TIMEOUT		= 0x0105,
BURNER_SCSI_FEAT_DVD_CSS		= 0x0106,
BURNER_SCSI_FEAT_REAL_TIME_STREAM	= 0x0107,
BURNER_SCSI_FEAT_DRIVE_SERIAL_NB	= 0x0108,
BURNER_SCSI_FEAT_MEDIA_SERIAL_NB	= 0x0109,
BURNER_SCSI_FEAT_DCB			= 0x010A,
BURNER_SCSI_FEAT_DVD_CPRM		= 0x010B,
BURNER_SCSI_FEAT_FIRMWARE_INFO		= 0x010C,
BURNER_SCSI_FEAT_AACS			= 0x010D,
	/* reserved */
BURNER_SCSI_FEAT_VCPS			= 0x0110,
} BurnerScsiFeatureType;


#if G_BYTE_ORDER == G_LITTLE_ENDIAN

struct _BurnerScsiFeatureDesc {
	uchar code		[2];

	uchar current		:1;
	uchar persistent	:1;
	uchar version		:4;
	uchar reserved		:2;

	uchar add_len;
	uchar data		[0];
};

struct _BurnerScsiCoreDescMMC4 {
	/* this is for MMC4 & MMC5 */
	uchar dbe		:1;
	uchar inq2		:1;
	uchar reserved0		:6;

	uchar reserved1		[3];
};

struct _BurnerScsiCoreDescMMC3 {
	uchar interface		[4];
};

struct _BurnerScsiProfileDesc {
	uchar number		[2];

	uchar currentp		:1;
	uchar reserved0		:7;

	uchar reserved1;
};

struct _BurnerScsiMorphingDesc {
	uchar async		:1;
	uchar op_chge_event	:1;
	uchar reserved0		:6;

	uchar reserved1		[3];
};

struct _BurnerScsiMediumDesc {
	uchar lock		:1;
	uchar reserved		:1;
	uchar prevent_jmp	:1;
	uchar eject		:1;
	uchar reserved1		:1;
	uchar loading_mech	:3;

	uchar reserved2		[3];
};

struct _BurnerScsiWrtProtectDesc {
	uchar sswpp		:1;
	uchar spwp		:1;
	uchar wdcb		:1;
	uchar dwp		:1;
	uchar reserved0		:4;

	uchar reserved1		[3];
};

struct _BurnerScsiRandomReadDesc {
	uchar block_size	[4];
	uchar blocking		[2];

	uchar pp		:1;
	uchar reserved0		:7;

	uchar reserved1;
};

struct _BurnerScsiCDReadDesc {
	uchar cdtext		:1;
	uchar c2flags		:1;
	uchar reserved0		:5;
	uchar dap		:1;

	uchar reserved1		[3];
};

/* MMC5 only otherwise just the header */
struct _BurnerScsiDVDReadDesc {
	uchar multi110		:1;
	uchar reserved0		:7;

	uchar reserved1;

	uchar dual_R		:1;
	uchar reserved2		:7;

	uchar reserved3;
};

struct _BurnerScsiRandomWriteDesc {
	/* MMC4/MMC5 only */
	uchar last_lba		[4];

	uchar block_size	[4];
	uchar blocking		[2];

	uchar pp		:1;
	uchar reserved0		:7;

	uchar reserved1;
};

struct _BurnerScsiIncrementalWrtDesc {
	uchar block_type	[2];

	uchar buf		:1;
	uchar arsv		:1;		/* MMC5 */
	uchar trio		:1;		/* MMC5 */
	uchar reserved0		:5;

	uchar num_link_sizes;
	uchar links		[0];
};

/* MMC5 only */
struct _BurnerScsiFormatDesc {
	uchar cert		:1;
	uchar qcert		:1;
	uchar expand		:1;
	uchar renosa		:1;
	uchar reserved0		:4;

	uchar reserved1		[3];

	uchar rrm		:1;
	uchar reserved2		:7;

	uchar reserved3		[3];
};

struct _BurnerScsiDefectMngDesc {
	uchar reserved0		:7;
	uchar ssa		:1;

	uchar reserved1		[3];
};

struct _BurnerScsiWrtOnceDesc {
	uchar lba_size		[4];
	uchar blocking		[2];

	uchar pp		:1;
	uchar reserved0		:7;

	uchar reserved1;
};

struct _BurnerScsiMRWDesc {
	uchar wrt_CD		:1;
	uchar rd_DVDplus	:1;
	uchar wrt_DVDplus	:1;
	uchar reserved0		:5;

	uchar reserved1		[3];
};

struct _BurnerScsiDefectReportDesc {
	uchar drt_dm		:1;
	uchar reserved0		:7;

	uchar dbi_zones_num;
	uchar num_entries	[2];
};

struct _BurnerScsiDVDRWplusDesc {
	uchar write		:1;
	uchar reserved0		:7;

	uchar close		:1;
	uchar quick_start	:1;
	uchar reserved1		:6;

	uchar reserved2		[2];
};

struct _BurnerScsiDVDRplusDesc {
	uchar write		:1;
	uchar reserved0		:7;

	uchar reserved1		[3];
};

struct _BurnerScsiRigidOverwrtDesc {
	uchar blank		:1;
	uchar intermediate	:1;
	uchar dsdr		:1;
	uchar dsdg		:1;
	uchar reserved0		:4;

	uchar reserved1		[3];
};

struct _BurnerScsiCDTAODesc {
	uchar RW_subcode	:1;
	uchar CDRW		:1;
	uchar dummy		:1;
	uchar RW_pack		:1;
	uchar RW_raw		:1;
	uchar reserved0		:1;
	uchar buf		:1;
	uchar reserved1		:1;

	uchar reserved2;

	uchar data_type		[2];
};

struct _BurnerScsiCDSAODesc {
	uchar rw_sub_chan	:1;
	uchar rw_CD		:1;
	uchar dummy		:1;
	uchar raw		:1;
	uchar raw_multi		:1;
	uchar sao		:1;
	uchar buf		:1;
	uchar reserved		:1;

	uchar max_cue_size	[3];
};

struct _BurnerScsiDVDRWlessWrtDesc {
	uchar reserved0		:1;
	uchar rw_DVD		:1;
	uchar dummy		:1;
	uchar dual_layer_r	:1;
	uchar reserved1		:2;
	uchar buf		:1;
	uchar reserved2		:1;

	uchar reserved3		[3];
};

struct _BurnerScsiCDRWWrtDesc {
	uchar reserved0;

	uchar sub0		:1;
	uchar sub1		:1;
	uchar sub2		:1;
	uchar sub3		:1;
	uchar sub4		:1;
	uchar sub5		:1;
	uchar sub6		:1;
	uchar sub7		:1;

	uchar reserved1		[2];
};

struct _BurnerScsiDVDRWDLDesc {
	uchar write		:1;
	uchar reserved0		:7;

	uchar close		:1;
	uchar quick_start	:1;
	uchar reserved1		:6;

	uchar reserved2		[2];
};

struct _BurnerScsiDVDRDLDesc {
	uchar write		:1;
	uchar reserved0		:7;

	uchar reserved1		[3];
};

struct _BurnerScsiBDReadDesc {
	uchar reserved		[4];

	uchar class0_RE_v8	:1;
	uchar class0_RE_v9	:1;
	uchar class0_RE_v10	:1;
	uchar class0_RE_v11	:1;
	uchar class0_RE_v12	:1;
	uchar class0_RE_v13	:1;
	uchar class0_RE_v14	:1;
	uchar class0_RE_v15	:1;

	uchar class0_RE_v0	:1;
	uchar class0_RE_v1	:1;
	uchar class0_RE_v2	:1;
	uchar class0_RE_v3	:1;
	uchar class0_RE_v4	:1;
	uchar class0_RE_v5	:1;
	uchar class0_RE_v6	:1;
	uchar class0_RE_v7	:1;
	
	uchar class1_RE_v8	:1;
	uchar class1_RE_v9	:1;
	uchar class1_RE_v10	:1;
	uchar class1_RE_v11	:1;
	uchar class1_RE_v12	:1;
	uchar class1_RE_v13	:1;
	uchar class1_RE_v14	:1;
	uchar class1_RE_v15	:1;
	
	uchar class1_RE_v0	:1;
	uchar class1_RE_v1	:1;
	uchar class1_RE_v2	:1;
	uchar class1_RE_v3	:1;
	uchar class1_RE_v4	:1;
	uchar class1_RE_v5	:1;
	uchar class1_RE_v6	:1;
	uchar class1_RE_v7	:1;
	
	uchar class2_RE_v8	:1;
	uchar class2_RE_v9	:1;
	uchar class2_RE_v10	:1;
	uchar class2_RE_v11	:1;
	uchar class2_RE_v12	:1;
	uchar class2_RE_v13	:1;
	uchar class2_RE_v14	:1;
	uchar class2_RE_v15	:1;
	
	uchar class2_RE_v0	:1;
	uchar class2_RE_v1	:1;
	uchar class2_RE_v2	:1;
	uchar class2_RE_v3	:1;
	uchar class2_RE_v4	:1;
	uchar class2_RE_v5	:1;
	uchar class2_RE_v6	:1;
	uchar class2_RE_v7	:1;
	
	uchar class3_RE_v8	:1;
	uchar class3_RE_v9	:1;
	uchar class3_RE_v10	:1;
	uchar class3_RE_v11	:1;
	uchar class3_RE_v12	:1;
	uchar class3_RE_v13	:1;
	uchar class3_RE_v14	:1;
	uchar class3_RE_v15	:1;
	
	uchar class3_RE_v0	:1;
	uchar class3_RE_v1	:1;
	uchar class3_RE_v2	:1;
	uchar class3_RE_v3	:1;
	uchar class3_RE_v4	:1;
	uchar class3_RE_v5	:1;
	uchar class3_RE_v6	:1;
	uchar class3_RE_v7	:1;

	uchar class0_R_v8	:1;
	uchar class0_R_v9	:1;
	uchar class0_R_v10	:1;
	uchar class0_R_v11	:1;
	uchar class0_R_v12	:1;
	uchar class0_R_v13	:1;
	uchar class0_R_v14	:1;
	uchar class0_R_v15	:1;
	
	uchar class0_R_v0	:1;
	uchar class0_R_v1	:1;
	uchar class0_R_v2	:1;
	uchar class0_R_v3	:1;
	uchar class0_R_v4	:1;
	uchar class0_R_v5	:1;
	uchar class0_R_v6	:1;
	uchar class0_R_v7	:1;
	
	uchar class1_R_v8	:1;
	uchar class1_R_v9	:1;
	uchar class1_R_v10	:1;
	uchar class1_R_v11	:1;
	uchar class1_R_v12	:1;
	uchar class1_R_v13	:1;
	uchar class1_R_v14	:1;
	uchar class1_R_v15	:1;
	
	uchar class1_R_v0	:1;
	uchar class1_R_v1	:1;
	uchar class1_R_v2	:1;
	uchar class1_R_v3	:1;
	uchar class1_R_v4	:1;
	uchar class1_R_v5	:1;
	uchar class1_R_v6	:1;
	uchar class1_R_v7	:1;
	
	uchar class2_R_v8	:1;
	uchar class2_R_v9	:1;
	uchar class2_R_v10	:1;
	uchar class2_R_v11	:1;
	uchar class2_R_v12	:1;
	uchar class2_R_v13	:1;
	uchar class2_R_v14	:1;
	uchar class2_R_v15	:1;
	
	uchar class2_R_v0	:1;
	uchar class2_R_v1	:1;
	uchar class2_R_v2	:1;
	uchar class2_R_v3	:1;
	uchar class2_R_v4	:1;
	uchar class2_R_v5	:1;
	uchar class2_R_v6	:1;
	uchar class2_R_v7	:1;
	
	uchar class3_R_v8	:1;
	uchar class3_R_v9	:1;
	uchar class3_R_v10	:1;
	uchar class3_R_v11	:1;
	uchar class3_R_v12	:1;
	uchar class3_R_v13	:1;
	uchar class3_R_v14	:1;
	uchar class3_R_v15	:1;
	
	uchar class3_R_v0	:1;
	uchar class3_R_v1	:1;
	uchar class3_R_v2	:1;
	uchar class3_R_v3	:1;
	uchar class3_R_v4	:1;
	uchar class3_R_v5	:1;
	uchar class3_R_v6	:1;
	uchar class3_R_v7	:1;
};

struct _BurnerScsiBDWriteDesc {
	uchar reserved		[4];

	uchar class0_RE_v8	:1;
	uchar class0_RE_v9	:1;
	uchar class0_RE_v10	:1;
	uchar class0_RE_v11	:1;
	uchar class0_RE_v12	:1;
	uchar class0_RE_v13	:1;
	uchar class0_RE_v14	:1;
	uchar class0_RE_v15	:1;
	
	uchar class0_RE_v0	:1;
	uchar class0_RE_v1	:1;
	uchar class0_RE_v2	:1;
	uchar class0_RE_v3	:1;
	uchar class0_RE_v4	:1;
	uchar class0_RE_v5	:1;
	uchar class0_RE_v6	:1;
	uchar class0_RE_v7	:1;
	
	uchar class1_RE_v8	:1;
	uchar class1_RE_v9	:1;
	uchar class1_RE_v10	:1;
	uchar class1_RE_v11	:1;
	uchar class1_RE_v12	:1;
	uchar class1_RE_v13	:1;
	uchar class1_RE_v14	:1;
	uchar class1_RE_v15	:1;
	
	uchar class1_RE_v0	:1;
	uchar class1_RE_v1	:1;
	uchar class1_RE_v2	:1;
	uchar class1_RE_v3	:1;
	uchar class1_RE_v4	:1;
	uchar class1_RE_v5	:1;
	uchar class1_RE_v6	:1;
	uchar class1_RE_v7	:1;
	
	uchar class2_RE_v8	:1;
	uchar class2_RE_v9	:1;
	uchar class2_RE_v10	:1;
	uchar class2_RE_v11	:1;
	uchar class2_RE_v12	:1;
	uchar class2_RE_v13	:1;
	uchar class2_RE_v14	:1;
	uchar class2_RE_v15	:1;
	
	uchar class2_RE_v0	:1;
	uchar class2_RE_v1	:1;
	uchar class2_RE_v2	:1;
	uchar class2_RE_v3	:1;
	uchar class2_RE_v4	:1;
	uchar class2_RE_v5	:1;
	uchar class2_RE_v6	:1;
	uchar class2_RE_v7	:1;
	
	uchar class3_RE_v8	:1;
	uchar class3_RE_v9	:1;
	uchar class3_RE_v10	:1;
	uchar class3_RE_v11	:1;
	uchar class3_RE_v12	:1;
	uchar class3_RE_v13	:1;
	uchar class3_RE_v14	:1;
	uchar class3_RE_v15	:1;
	
	uchar class3_RE_v0	:1;
	uchar class3_RE_v1	:1;
	uchar class3_RE_v2	:1;
	uchar class3_RE_v3	:1;
	uchar class3_RE_v4	:1;
	uchar class3_RE_v5	:1;
	uchar class3_RE_v6	:1;
	uchar class3_RE_v7	:1;

	uchar class0_R_v8	:1;
	uchar class0_R_v9	:1;
	uchar class0_R_v10	:1;
	uchar class0_R_v11	:1;
	uchar class0_R_v12	:1;
	uchar class0_R_v13	:1;
	uchar class0_R_v14	:1;
	uchar class0_R_v15	:1;
	
	uchar class0_R_v0	:1;
	uchar class0_R_v1	:1;
	uchar class0_R_v2	:1;
	uchar class0_R_v3	:1;
	uchar class0_R_v4	:1;
	uchar class0_R_v5	:1;
	uchar class0_R_v6	:1;
	uchar class0_R_v7	:1;
	
	uchar class1_R_v8	:1;
	uchar class1_R_v9	:1;
	uchar class1_R_v10	:1;
	uchar class1_R_v11	:1;
	uchar class1_R_v12	:1;
	uchar class1_R_v13	:1;
	uchar class1_R_v14	:1;
	uchar class1_R_v15	:1;
	
	uchar class1_R_v0	:1;
	uchar class1_R_v1	:1;
	uchar class1_R_v2	:1;
	uchar class1_R_v3	:1;
	uchar class1_R_v4	:1;
	uchar class1_R_v5	:1;
	uchar class1_R_v6	:1;
	uchar class1_R_v7	:1;
	
	uchar class2_R_v8	:1;
	uchar class2_R_v9	:1;
	uchar class2_R_v10	:1;
	uchar class2_R_v11	:1;
	uchar class2_R_v12	:1;
	uchar class2_R_v13	:1;
	uchar class2_R_v14	:1;
	uchar class2_R_v15	:1;
	
	uchar class2_R_v0	:1;
	uchar class2_R_v1	:1;
	uchar class2_R_v2	:1;
	uchar class2_R_v3	:1;
	uchar class2_R_v4	:1;
	uchar class2_R_v5	:1;
	uchar class2_R_v6	:1;
	uchar class2_R_v7	:1;
	
	uchar class3_R_v8	:1;
	uchar class3_R_v9	:1;
	uchar class3_R_v10	:1;
	uchar class3_R_v11	:1;
	uchar class3_R_v12	:1;
	uchar class3_R_v13	:1;
	uchar class3_R_v14	:1;
	uchar class3_R_v15	:1;
	
	uchar class3_R_v0	:1;
	uchar class3_R_v1	:1;
	uchar class3_R_v2	:1;
	uchar class3_R_v3	:1;
	uchar class3_R_v4	:1;
	uchar class3_R_v5	:1;
	uchar class3_R_v6	:1;
	uchar class3_R_v7	:1;
};

struct _BurnerScsiHDDVDReadDesc {
	uchar hd_dvd_r		:1;
	uchar reserved0		:7;

	uchar reserved1;

	uchar hd_dvd_ram	:1;
	uchar reserved2		:7;

	uchar reserved3;
};

struct _BurnerScsiHDDVDWriteDesc {
	uchar hd_dvd_r		:1;
	uchar reserved0		:7;

	uchar reserved1;

	uchar hd_dvd_ram	:1;
	uchar reserved2		:7;

	uchar reserved3;
};

struct _BurnerScsiHybridDiscDesc {
	uchar ri		:1;
	uchar reserved0		:7;

	uchar reserved1		[3];
};

struct _BurnerScsiSmartDesc {
	uchar pp		:1;
	uchar reserved0		:7;

	uchar reserved1		[3];
};

struct _BurnerScsiEmbedChngDesc {
	uchar reserved0		:1;
	uchar sdp		:1;
	uchar reserved1		:1;
	uchar scc		:1;
	uchar reserved2		:3;

	uchar reserved3		[2];

	uchar slot_num		:5;
	uchar reserved4		:3;
};

struct _BurnerScsiExtAudioPlayDesc {
	uchar separate_vol	:1;
	uchar separate_chnl_mute:1;
	uchar scan_command	:1;
	uchar reserved0		:5;

	uchar reserved1;

	uchar number_vol	[2];
};

struct _BurnerScsiFirmwareUpgrDesc {
	uchar m5		:1;
	uchar reserved0		:7;

	uchar reserved1		[3];
};

struct _BurnerScsiTimeoutDesc {
	uchar group3		:1;
	uchar reserved0		:7;

	uchar reserved1;
	uchar unit_len		[2];
};

struct _BurnerScsiRTStreamDesc {
	uchar stream_wrt	:1;
	uchar wrt_spd		:1;
	uchar mp2a		:1;
	uchar set_cd_spd	:1;
	uchar rd_buf_caps_block	:1;
	uchar reserved0		:3;

	uchar reserved1		[3];
};

struct _BurnerScsiAACSDesc {
	uchar bng		:1;
	uchar reserved0		:7;

	uchar block_count;

	uchar agids_num		:4;
	uchar reserved1		:4;

	uchar version;
};

#else

struct _BurnerScsiFeatureDesc {
	uchar code		[2];

	uchar current		:1;
	uchar persistent	:1;
	uchar version		:4;
	uchar reserved		:2;

	uchar add_len;
	uchar data		[0];
};

struct _BurnerScsiProfileDesc {
	uchar number		[2];

	uchar reserved0		:7;
	uchar currentp		:1;

	uchar reserved1;
};

struct _BurnerScsiCoreDescMMC4 {
	uchar reserved0		:6;
	uchar inq2		:1;
	uchar dbe		:1;

  	uchar mmc4		[0];
	uchar reserved1		[3];
};

struct _BurnerScsiCoreDescMMC3 {
	uchar interface		[4];
};

struct _BurnerScsiMorphingDesc {
	uchar reserved0		:6;
	uchar op_chge_event	:1;
	uchar async		:1;

	uchar reserved1		[3];
};

struct _BurnerScsiMediumDesc {
	uchar loading_mech	:3;
	uchar reserved1		:1;
	uchar eject		:1;
	uchar prevent_jmp	:1;
	uchar reserved		:1;
	uchar lock		:1;

	uchar reserved2		[3];
};

struct _BurnerScsiWrtProtectDesc {
	uchar reserved0		:4;
	uchar dwp		:1;
	uchar wdcb		:1;
	uchar spwp		:1;
	uchar sswpp		:1;

	uchar reserved1		[3];
};

struct _BurnerScsiRandomReadDesc {
	uchar block_size	[4];
	uchar blocking		[2];

	uchar reserved0		:7;
	uchar pp		:1;

	uchar reserved1;
};

struct _BurnerScsiCDReadDesc {
	uchar dap		:1;
	uchar reserved0		:5;
	uchar c2flags		:1;
	uchar cdtext		:1;

	uchar reserved1		[3];
};

struct _BurnerScsiDVDReadDesc {
	uchar reserved0		:7;
	uchar multi110		:1;

	uchar reserved1;

	uchar reserved2		:7;
	uchar dual_R		:1;

	uchar reserved3;
};

struct _BurnerScsiRandomWriteDesc {
	uchar last_lba		[4];
	uchar block_size	[4];
	uchar blocking		[2];

	uchar reserved0		:7;
	uchar pp		:1;

	uchar reserved1;
};

struct _BurnerScsiIncrementalWrtDesc {
	uchar block_type	[2];

	uchar reserved0		:5;
	uchar trio		:1;
	uchar arsv		:1;
	uchar buf		:1;

	uchar num_link_sizes;
	uchar links;
};

struct _BurnerScsiFormatDesc {
	uchar reserved0		:4;
	uchar renosa		:1;
	uchar expand		:1;
	uchar qcert		:1;
	uchar cert		:1;

	uchar reserved1		[3];

	uchar reserved2		:7;
	uchar rrm		:1;

	uchar reserved3		[3];
};

struct _BurnerScsiDefectMngDesc {
	uchar ssa		:1;
	uchar reserved0		:7;

	uchar reserved1		[3];
};

struct _BurnerScsiWrtOnceDesc {
	uchar lba_size		[4];
	uchar blocking		[2];

	uchar reserved0		:7;
	uchar pp		:1;

	uchar reserved1;
};

struct _BurnerScsiMRWDesc {
	uchar reserved0		:5;
	uchar wrt_DVDplus	:1;
	uchar rd_DVDplus	:1;
	uchar wrt_CD		:1;

	uchar reserved1		[3];
};

struct _BurnerScsiDefectReportDesc {
	uchar reserved0		:7;
	uchar drt_dm		:1;

	uchar dbi_zones_num;
	uchar num_entries	[2];
};

struct _BurnerScsiDVDRWplusDesc {
	uchar reserved0		:7;
	uchar write		:1;

	uchar reserved1		:6;
	uchar quick_start	:1;
	uchar close		:1;

	uchar reserved2		[2];
};

struct _BurnerScsiDVDRplusDesc {
	uchar reserved0		:7;
	uchar write		:1;

	uchar reserved1		[3];
};

struct _BurnerScsiRigidOverwrtDesc {
	uchar reserved0		:4;
	uchar dsdg		:1;
	uchar dsdr		:1;
	uchar intermediate	:1;
	uchar blank		:1;

	uchar reserved1		[3];
};

struct _BurnerScsiCDTAODesc {
	uchar reserved1		:1;
	uchar buf		:1;
	uchar reserved0		:1;
	uchar RW_raw		:1;
	uchar RW_pack		:1;
	uchar dummy		:1;
	uchar CDRW		:1;
	uchar RW_subcode	:1;

	uchar reserved2;

	uchar data_type		[2];
};

struct _BurnerScsiCDSAODesc {
	uchar reserved		:1;
	uchar buf		:1;
	uchar sao		:1;
	uchar raw_multi		:1;
	uchar raw		:1;
	uchar dummy		:1;
	uchar rw_CD		:1;
	uchar rw_sub_chan	:1;

	uchar max_cue_size	[3];
};

struct _BurnerScsiDVDRWlessWrtDesc {
	uchar reserved2		:1;
	uchar buf		:1;
	uchar reserved1		:2;
	uchar dual_layer_r	:1;
	uchar dummy		:1;
	uchar rw_DVD		:1;
	uchar reserved0		:1;

	uchar reserved3		[3];
};

struct _BurnerScsiCDRWWrtDesc {
	uchar reserved0;

	uchar sub7		:1;
	uchar sub6		:1;
	uchar sub5		:1;
	uchar sub4		:1;
	uchar sub3		:1;
	uchar sub2		:1;
	uchar sub1		:1;
	uchar sub0		:1;

	uchar reserved1		[2];
};

struct _BurnerScsiDVDRWDLDesc {
	uchar reserved0		:7;
	uchar write		:1;

	uchar reserved1		:6;
	uchar quick_start	:1;
	uchar close		:1;

	uchar reserved2		[2];
};

struct _BurnerScsiDVDRDLDesc {
	uchar reserved0		:7;
	uchar write		:1;

	uchar reserved1		[3];
};

struct _BurnerScsiBDReadDesc {
	uchar reserved		[4];

	uchar class0_RE_v15	:1;
	uchar class0_RE_v14	:1;
	uchar class0_RE_v13	:1;
	uchar class0_RE_v12	:1;
	uchar class0_RE_v11	:1;
	uchar class0_RE_v10	:1;
	uchar class0_RE_v9	:1;
	uchar class0_RE_v8	:1;

	uchar class0_RE_v7	:1;
	uchar class0_RE_v6	:1;
	uchar class0_RE_v5	:1;
	uchar class0_RE_v4	:1;
	uchar class0_RE_v3	:1;
	uchar class0_RE_v2	:1;
	uchar class0_RE_v1	:1;
	uchar class0_RE_v0	:1;

	uchar class1_RE_v15	:1;
	uchar class1_RE_v14	:1;
	uchar class1_RE_v13	:1;
	uchar class1_RE_v12	:1;
	uchar class1_RE_v11	:1;
	uchar class1_RE_v10	:1;
	uchar class1_RE_v9	:1;
	uchar class1_RE_v8	:1;
	
	uchar class1_RE_v7	:1;
	uchar class1_RE_v6	:1;
	uchar class1_RE_v5	:1;
	uchar class1_RE_v4	:1;
	uchar class1_RE_v3	:1;
	uchar class1_RE_v2	:1;
	uchar class1_RE_v1	:1;
	uchar class1_RE_v0	:1;
	
	uchar class2_RE_v15	:1;
	uchar class2_RE_v14	:1;
	uchar class2_RE_v13	:1;
	uchar class2_RE_v12	:1;
	uchar class2_RE_v11	:1;
	uchar class2_RE_v10	:1;
	uchar class2_RE_v9	:1;
	uchar class2_RE_v8	:1;
	
	uchar class2_RE_v7	:1;
	uchar class2_RE_v6	:1;
	uchar class2_RE_v5	:1;
	uchar class2_RE_v4	:1;
	uchar class2_RE_v3	:1;
	uchar class2_RE_v2	:1;
	uchar class2_RE_v1	:1;
	uchar class2_RE_v0	:1;
	
	uchar class3_RE_v15	:1;
	uchar class3_RE_v14	:1;
	uchar class3_RE_v13	:1;
	uchar class3_RE_v12	:1;
	uchar class3_RE_v11	:1;
	uchar class3_RE_v10	:1;
	uchar class3_RE_v9	:1;
	uchar class3_RE_v8	:1;
	
	uchar class3_RE_v7	:1;
	uchar class3_RE_v6	:1;
	uchar class3_RE_v5	:1;
	uchar class3_RE_v4	:1;
	uchar class3_RE_v3	:1;
	uchar class3_RE_v2	:1;
	uchar class3_RE_v1	:1;
	uchar class3_RE_v0	:1;

	uchar class0_R_v15	:1;
	uchar class0_R_v14	:1;
	uchar class0_R_v13	:1;
	uchar class0_R_v12	:1;
	uchar class0_R_v11	:1;
	uchar class0_R_v10	:1;
	uchar class0_R_v9	:1;
	uchar class0_R_v8	:1;

	uchar class0_R_v7	:1;
	uchar class0_R_v6	:1;
	uchar class0_R_v5	:1;
	uchar class0_R_v4	:1;
	uchar class0_R_v3	:1;
	uchar class0_R_v2	:1;
	uchar class0_R_v1	:1;
	uchar class0_R_v0	:1;

	uchar class1_R_v15	:1;
	uchar class1_R_v14	:1;
	uchar class1_R_v13	:1;
	uchar class1_R_v12	:1;
	uchar class1_R_v11	:1;
	uchar class1_R_v10	:1;
	uchar class1_R_v9	:1;
	uchar class1_R_v8	:1;
	
	uchar class1_R_v7	:1;
	uchar class1_R_v6	:1;
	uchar class1_R_v5	:1;
	uchar class1_R_v4	:1;
	uchar class1_R_v3	:1;
	uchar class1_R_v2	:1;
	uchar class1_R_v1	:1;
	uchar class1_R_v0	:1;
	
	uchar class2_R_v15	:1;
	uchar class2_R_v14	:1;
	uchar class2_R_v13	:1;
	uchar class2_R_v12	:1;
	uchar class2_R_v11	:1;
	uchar class2_R_v10	:1;
	uchar class2_R_v9	:1;
	uchar class2_R_v8	:1;
	
	uchar class2_R_v7	:1;
	uchar class2_R_v6	:1;
	uchar class2_R_v5	:1;
	uchar class2_R_v4	:1;
	uchar class2_R_v3	:1;
	uchar class2_R_v2	:1;
	uchar class2_R_v1	:1;
	uchar class2_R_v0	:1;
	
	uchar class3_R_v15	:1;
	uchar class3_R_v14	:1;
	uchar class3_R_v13	:1;
	uchar class3_R_v12	:1;
	uchar class3_R_v11	:1;
	uchar class3_R_v10	:1;
	uchar class3_R_v9	:1;
	uchar class3_R_v8	:1;
	
	uchar class3_R_v7	:1;
	uchar class3_R_v6	:1;
	uchar class3_R_v5	:1;
	uchar class3_R_v4	:1;
	uchar class3_R_v3	:1;
	uchar class3_R_v2	:1;
	uchar class3_R_v1	:1;
	uchar class3_R_v0	:1;
};

struct _BurnerScsiBDWriteDesc {
	uchar reserved		[4];

	uchar class0_RE_v15	:1;
	uchar class0_RE_v14	:1;
	uchar class0_RE_v13	:1;
	uchar class0_RE_v12	:1;
	uchar class0_RE_v11	:1;
	uchar class0_RE_v10	:1;
	uchar class0_RE_v9	:1;
	uchar class0_RE_v8	:1;

	uchar class0_RE_v7	:1;
	uchar class0_RE_v6	:1;
	uchar class0_RE_v5	:1;
	uchar class0_RE_v4	:1;
	uchar class0_RE_v3	:1;
	uchar class0_RE_v2	:1;
	uchar class0_RE_v1	:1;
	uchar class0_RE_v0	:1;

	uchar class1_RE_v15	:1;
	uchar class1_RE_v14	:1;
	uchar class1_RE_v13	:1;
	uchar class1_RE_v12	:1;
	uchar class1_RE_v11	:1;
	uchar class1_RE_v10	:1;
	uchar class1_RE_v9	:1;
	uchar class1_RE_v8	:1;
	
	uchar class1_RE_v7	:1;
	uchar class1_RE_v6	:1;
	uchar class1_RE_v5	:1;
	uchar class1_RE_v4	:1;
	uchar class1_RE_v3	:1;
	uchar class1_RE_v2	:1;
	uchar class1_RE_v1	:1;
	uchar class1_RE_v0	:1;
	
	uchar class2_RE_v15	:1;
	uchar class2_RE_v14	:1;
	uchar class2_RE_v13	:1;
	uchar class2_RE_v12	:1;
	uchar class2_RE_v11	:1;
	uchar class2_RE_v10	:1;
	uchar class2_RE_v9	:1;
	uchar class2_RE_v8	:1;
	
	uchar class2_RE_v7	:1;
	uchar class2_RE_v6	:1;
	uchar class2_RE_v5	:1;
	uchar class2_RE_v4	:1;
	uchar class2_RE_v3	:1;
	uchar class2_RE_v2	:1;
	uchar class2_RE_v1	:1;
	uchar class2_RE_v0	:1;
	
	uchar class3_RE_v15	:1;
	uchar class3_RE_v14	:1;
	uchar class3_RE_v13	:1;
	uchar class3_RE_v12	:1;
	uchar class3_RE_v11	:1;
	uchar class3_RE_v10	:1;
	uchar class3_RE_v9	:1;
	uchar class3_RE_v8	:1;
	
	uchar class3_RE_v7	:1;
	uchar class3_RE_v6	:1;
	uchar class3_RE_v5	:1;
	uchar class3_RE_v4	:1;
	uchar class3_RE_v3	:1;
	uchar class3_RE_v2	:1;
	uchar class3_RE_v1	:1;
	uchar class3_RE_v0	:1;

	uchar class0_R_v15	:1;
	uchar class0_R_v14	:1;
	uchar class0_R_v13	:1;
	uchar class0_R_v12	:1;
	uchar class0_R_v11	:1;
	uchar class0_R_v10	:1;
	uchar class0_R_v9	:1;
	uchar class0_R_v8	:1;

	uchar class0_R_v7	:1;
	uchar class0_R_v6	:1;
	uchar class0_R_v5	:1;
	uchar class0_R_v4	:1;
	uchar class0_R_v3	:1;
	uchar class0_R_v2	:1;
	uchar class0_R_v1	:1;
	uchar class0_R_v0	:1;

	uchar class1_R_v15	:1;
	uchar class1_R_v14	:1;
	uchar class1_R_v13	:1;
	uchar class1_R_v12	:1;
	uchar class1_R_v11	:1;
	uchar class1_R_v10	:1;
	uchar class1_R_v9	:1;
	uchar class1_R_v8	:1;
	
	uchar class1_R_v7	:1;
	uchar class1_R_v6	:1;
	uchar class1_R_v5	:1;
	uchar class1_R_v4	:1;
	uchar class1_R_v3	:1;
	uchar class1_R_v2	:1;
	uchar class1_R_v1	:1;
	uchar class1_R_v0	:1;
	
	uchar class2_R_v15	:1;
	uchar class2_R_v14	:1;
	uchar class2_R_v13	:1;
	uchar class2_R_v12	:1;
	uchar class2_R_v11	:1;
	uchar class2_R_v10	:1;
	uchar class2_R_v9	:1;
	uchar class2_R_v8	:1;
	
	uchar class2_R_v7	:1;
	uchar class2_R_v6	:1;
	uchar class2_R_v5	:1;
	uchar class2_R_v4	:1;
	uchar class2_R_v3	:1;
	uchar class2_R_v2	:1;
	uchar class2_R_v1	:1;
	uchar class2_R_v0	:1;
	
	uchar class3_R_v15	:1;
	uchar class3_R_v14	:1;
	uchar class3_R_v13	:1;
	uchar class3_R_v12	:1;
	uchar class3_R_v11	:1;
	uchar class3_R_v10	:1;
	uchar class3_R_v9	:1;
	uchar class3_R_v8	:1;
	
	uchar class3_R_v7	:1;
	uchar class3_R_v6	:1;
	uchar class3_R_v5	:1;
	uchar class3_R_v4	:1;
	uchar class3_R_v3	:1;
	uchar class3_R_v2	:1;
	uchar class3_R_v1	:1;
	uchar class3_R_v0	:1;
};

struct _BurnerScsiHDDVDReadDesc {
	uchar reserved0		:7;
	uchar hd_dvd_r		:1;

	uchar reserved1;

	uchar reserved2		:7;
	uchar hd_dvd_ram	:1;

	uchar reserved3;
};

struct _BurnerScsiHDDVDWriteDesc {
	uchar reserved0		:7;
	uchar hd_dvd_r		:1;

	uchar reserved1;

	uchar reserved2		:7;
	uchar hd_dvd_ram	:1;

	uchar reserved3;
};

struct _BurnerScsiHybridDiscDesc {
	uchar reserved0		:7;
	uchar ri		:1;

	uchar reserved1		[3];
};

struct _BurnerScsiSmartDesc {
	uchar reserved0		:7;
	uchar pp		:1;

	uchar reserved1		[3];
};

struct _BurnerScsiEmbedChngDesc {
	uchar reserved2		:3;
	uchar scc		:1;
	uchar reserved1		:1;
	uchar sdp		:1;
	uchar reserved0		:1;

	uchar reserved3		[2];

	uchar reserved4		:3;
	uchar slot_num		:5;
};

struct _BurnerScsiExtAudioPlayDesc {
	uchar reserved0		:5;
	uchar scan_command	:1;
	uchar separate_chnl_mute:1;
	uchar separate_vol	:1;

	uchar reserved1;

	uchar number_vol	[2];
};

struct _BurnerScsiFirmwareUpgrDesc {
	uchar reserved0		:7;
	uchar m5		:1;

	uchar reserved1		[3];
};

struct _BurnerScsiTimeoutDesc {
	uchar reserved0		:7;
	uchar group3		:1;

	uchar reserved1;
	uchar unit_len		[2];
};

struct _BurnerScsiRTStreamDesc {
	uchar reserved0		:3;
	uchar rd_buf_caps_block	:1;
	uchar set_cd_spd	:1;
	uchar mp2a		:1;
	uchar wrt_spd		:1;
	uchar stream_wrt	:1;

	uchar reserved1		[3];
};

struct _BurnerScsiAACSDesc {
	uchar reserved0		:7;
	uchar bng		:1;

	uchar block_count;

	uchar reserved1		:4;
	uchar agids_num		:4;

	uchar version;
};

#endif

struct _BurnerScsiInterfaceDesc {
	uchar code		[4];
};

struct _BurnerScsiCDrwCavDesc {
	uchar reserved		[4];
};

/* NOTE: this structure is extendable with padding to have a multiple of 4 */
struct _BurnerScsiLayerJmpDesc {
	uchar reserved0		[3];
	uchar num_link_sizes;
	uchar links		[0];
};

struct _BurnerScsiPOWDesc {
	uchar reserved		[4];
};

struct _BurnerScsiDVDCssDesc {
	uchar reserved		[3];
	uchar version;
};

/* NOTE: this structure is extendable with padding to have a multiple of 4 */
struct _BurnerScsiDriveSerialNumDesc {
	uchar serial		[4];
};

struct _BurnerScsiMediaSerialNumDesc {
	uchar serial		[4];
};

/* NOTE: this structure is extendable with padding to have a multiple of 4 */
struct _BurnerScsiDiscCtlBlocksDesc {
	uchar entry		[1][4];
};

struct _BurnerScsiDVDCprmDesc {
	uchar reserved0 	[3];
	uchar version;
};

struct _BurnerScsiFirmwareDesc {
	uchar century		[2];
	uchar year		[2];
	uchar month		[2];
	uchar day		[2];
	uchar hour		[2];
	uchar minute		[2];
	uchar second		[2];
	uchar reserved		[2];
};

struct _BurnerScsiVPSDesc {
	uchar reserved		[4];
};

typedef struct _BurnerScsiFeatureDesc BurnerScsiFeatureDesc;
typedef struct _BurnerScsiProfileDesc BurnerScsiProfileDesc;
typedef struct _BurnerScsiCoreDescMMC3 BurnerScsiCoreDescMMC3;
typedef struct _BurnerScsiCoreDescMMC4 BurnerScsiCoreDescMMC4;
typedef struct _BurnerScsiInterfaceDesc BurnerScsiInterfaceDesc;
typedef struct _BurnerScsiMorphingDesc BurnerScsiMorphingDesc;
typedef struct _BurnerScsiMediumDesc BurnerScsiMediumDesc;
typedef struct _BurnerScsiWrtProtectDesc BurnerScsiWrtProtectDesc;
typedef struct _BurnerScsiRandomReadDesc BurnerScsiRandomReadDesc;
typedef struct _BurnerScsiCDReadDesc BurnerScsiCDReadDesc;
typedef struct _BurnerScsiDVDReadDesc BurnerScsiDVDReadDesc;
typedef struct _BurnerScsiRandomWriteDesc BurnerScsiRandomWriteDesc;
typedef struct _BurnerScsiIncrementalWrtDesc BurnerScsiIncrementalWrtDesc;
typedef struct _BurnerScsiFormatDesc BurnerScsiFormatDesc;
typedef struct _BurnerScsiDefectMngDesc BurnerScsiDefectMngDesc;
typedef struct _BurnerScsiWrtOnceDesc BurnerScsiWrtOnceDesc;
typedef struct _BurnerScsiCDrwCavDesc BurnerScsiCDrwCavDesc;
typedef struct _BurnerScsiMRWDesc BurnerScsiMRWDesc;
typedef struct _BurnerScsiDefectReportDesc BurnerScsiDefectReportDesc;
typedef struct _BurnerScsiDVDRWplusDesc BurnerScsiDVDRWplusDesc;
typedef struct _BurnerScsiDVDRplusDesc BurnerScsiDVDRplusDesc;
typedef struct _BurnerScsiRigidOverwrtDesc BurnerScsiRigidOverwrtDesc;
typedef struct _BurnerScsiCDTAODesc BurnerScsiCDTAODesc;
typedef struct _BurnerScsiCDSAODesc BurnerScsiCDSAODesc;
typedef struct _BurnerScsiDVDRWlessWrtDesc BurnerScsiDVDRWlessWrtDesc;
typedef struct _BurnerScsiLayerJmpDesc BurnerScsiLayerJmpDesc;
typedef struct _BurnerScsiCDRWWrtDesc BurnerScsiCDRWWrtDesc;
typedef struct _BurnerScsiDVDRWDLDesc BurnerScsiDVDRWDLDesc;
typedef struct _BurnerScsiDVDRDLDesc BurnerScsiDVDRDLDesc;
typedef struct _BurnerScsiBDReadDesc BurnerScsiBDReadDesc;
typedef struct _BurnerScsiBDWriteDesc BurnerScsiBDWriteDesc;
typedef struct _BurnerScsiHDDVDReadDesc BurnerScsiHDDVDReadDesc;
typedef struct _BurnerScsiHDDVDWriteDesc BurnerScsiHDDVDWriteDesc;
typedef struct _BurnerScsiHybridDiscDesc BurnerScsiHybridDiscDesc;
typedef struct _BurnerScsiSmartDesc BurnerScsiSmartDesc;
typedef struct _BurnerScsiEmbedChngDesc BurnerScsiEmbedChngDesc;
typedef struct _BurnerScsiExtAudioPlayDesc BurnerScsiExtAudioPlayDesc;
typedef struct _BurnerScsiFirmwareUpgrDesc BurnerScsiFirmwareUpgrDesc;
typedef struct _BurnerScsiTimeoutDesc BurnerScsiTimeoutDesc;
typedef struct _BurnerScsiRTStreamDesc BurnerScsiRTStreamDesc;
typedef struct _BurnerScsiAACSDesc BurnerScsiAACSDesc;
typedef struct _BurnerScsiPOWDesc BurnerScsiPOWDesc;
typedef struct _BurnerScsiDVDCssDesc BurnerScsiDVDCssDesc;
typedef struct _BurnerScsiDriveSerialNumDesc BurnerScsiDriveSerialNumDesc;
typedef struct _BurnerScsiMediaSerialNumDesc BurnerScsiMediaSerialNumDesc;
typedef struct _BurnerScsiDiscCtlBlocksDesc BurnerScsiDiscCtlBlocksDesc;
typedef struct _BurnerScsiDVDCprmDesc BurnerScsiDVDCprmDesc;
typedef struct _BurnerScsiFirmwareDesc BurnerScsiFirmwareDesc;
typedef struct _BurnerScsiVPSDesc BurnerScsiVPSDesc;

struct _BurnerScsiGetConfigHdr {
	uchar len			[4];
	uchar reserved			[2];
	uchar current_profile		[2];

	BurnerScsiFeatureDesc desc 	[0];
};
typedef struct _BurnerScsiGetConfigHdr BurnerScsiGetConfigHdr;

G_END_DECLS

#endif /* _SCSI_GET_CONFIGURATION_H */

 
