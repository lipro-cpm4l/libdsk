/***************************************************************************
 *                                                                         *
 *    LIBDSK: General floppy and diskimage access library                  *
 *    Copyright (C) 2001  John Elliott <jce@seasip.demon.co.uk>            *
 *                                                                         *
 *    This library is free software; you can redistribute it and/or        *
 *    modify it under the terms of the GNU Library General Public          *
 *    License as published by the Free Software Foundation; either         *
 *    version 2 of the License, or (at your option) any later version.     *
 *                                                                         *
 *    This library is distributed in the hope that it will be useful,      *
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of       *
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU    *
 *    Library General Public License for more details.                     *
 *                                                                         *
 *    You should have received a copy of the GNU Library General Public    *
 *    License along with this library; if not, write to the Free           *
 *    Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,      *
 *    MA 02111-1307, USA                                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef LIBDSK_H
#define LIBDSK_H 1
/*
 * LIBDSK: Portable library for floppy / disk image access
 *
 */

#ifdef __cplusplus
extern "C" {
#endif

/************************* TYPES ********************************/

typedef          int  dsk_err_t;	/* Error number */
typedef unsigned long dsk_lsect_t;	/* Logical sector */
typedef unsigned int  dsk_ltrack_t;	/* Logical track */
typedef unsigned int  dsk_psect_t;	/* Physical sector */
typedef unsigned int  dsk_pcyl_t;	/* Physical cylinder */
typedef unsigned int  dsk_phead_t;	/* Physical head */
typedef unsigned char dsk_gap_t;	/* Gap length */
typedef const char *  dsk_cchar_t;	/* Const char * */
#define DSK_ERR_OK	 (0)	/* No error */
#define DSK_ERR_BADPTR	 (-1)	/* Bad pointer */
#define DSK_ERR_DIVZERO  (-2)	/* Division by zero */
#define DSK_ERR_BADPARM  (-3)	/* Bad parameter */
#define DSK_ERR_NODRVR   (-4)	/* Driver not found */
#define DSK_ERR_NOTME    (-5)	/* Driver rejects disc */
#define DSK_ERR_SYSERR   (-6)	/* System error, use errno */
#define DSK_ERR_NOMEM  	 (-7)	/* Null return from malloc */
#define DSK_ERR_NOTIMPL  (-8)	/* Function not implemented */
#define DSK_ERR_MISMATCH (-9)	/* Check sectors: No match*/
#define DSK_ERR_NOTRDY   (-10)	/* Drive not ready */
#define DSK_ERR_RDONLY	 (-11)	/* Read-only disc */
#define DSK_ERR_SEEKFAIL (-12)	/* Seek fail */
#define DSK_ERR_DATAERR  (-13)	/* CRC data error */
#define DSK_ERR_NODATA   (-14)	/* No data */
#define DSK_ERR_NOADDR	 (-15)	/* Missing address mark */
#define DSK_ERR_BADFMT   (-16)	/* Bad format */
#define DSK_ERR_CHANGED  (-19)	/* Disc changed */
#define DSK_ERR_ECHECK	 (-20)	/* Equipment check */
#define DSK_ERR_OVERRUN  (-21)	/* Overrun */
#define DSK_ERR_ACCESS   (-22)	/* Access denied */
#define DSK_ERR_CTRLR 	 (-23)	/* Failed controller */
#define DSK_ERR_UNKNOWN  (-99)	/* Unknown error */

/* Disc sidedness (logical/physical mapping). Use SIDES_ALT for single-sided floppies. */
typedef enum 
{	
	SIDES_ALT, 	/* Track n is cylinder (n/heads) head (n%heads) */
	SIDES_OUTBACK, 	/* Tracks go (head 0) 0,1,2,3,...37,38,39, then
			             (head 1) 39,38,37,...,2,1,0 */
	SIDES_OUTOUT	/* Tracks go (head 0) 0,1,2,3,...37,38,39, then
			             (head 1) 0,1,2,3,...37,38,39 */
} dsk_sides_t;

typedef enum
{
	RATE_HD,	/* Data rate for 1.4Mb 3.5"  in 3.5"  drive */
	RATE_DD,	/* Data rate for 360k  5.25" in 1.2Mb drive */
	RATE_SD,	/* Data rate for 720k  3.5"  in 3.5"  drive */
	RATE_ED		/* Data rate for 2.8Mb 3.5"  in 3.5"  drive */
} dsk_rate_t;

typedef enum
{
	FMT_180K,	/* 9 sectors, 1 side */
	FMT_CPCSYS,	/* CPC system 180K */
	FMT_CPCDATA,	/* CPC data 180K */
	FMT_720K,	/* 9 sectors, 80 tracks, 2 sides */
	FMT_1440K,	/* 1.4M */
	FMT_160K,	/* 8 sectors, 1 side */
	FMT_320K,	/* 8 sectors, 2 sides */
	FMT_360K,	/* 9 sectors, 2 sides */
	FMT_720F,	/* 9 sectors, 2 sides out-and-back */
	FMT_1200F,	/* 15 sectors, 2 sides out-and-back */
	FMT_1440F,	/* 18 sectors, 2 sides out-and-back */
	FMT_ACORN160,	/* 16 sectors,  256 bytes/sector, 1 side */
	FMT_ACORN320,	/* 16 sectors,  256 bytes/sector, 1 side */
	FMT_ACORN640,	/* 16 sectors,  256 bytes/sector, 2 sides */
	FMT_ACORN800,	/*  5 sectors, 1024 bytes/sector, 2 sides */
	FMT_ACORN1600,	/* 10 sectors, 1024 bytes/sector, 2 sides */
	FMT_800K,	/* 10 sectors, 80 tracks, 2 sides */
	FMT_200K,	/* 10 sectors, 40 tracks, 1 side */
} dsk_format_t;

/* DSK_GEOMETRY holds information used to convert physical to/from logical
 * sectors and to access the disc */
typedef struct
{
	dsk_sides_t	dg_sidedness;	/* How to handle multisided discs? */
	dsk_pcyl_t  	dg_cylinders;	/* Number of cylinders */
	dsk_phead_t	dg_heads;	/* Number of heads */
	dsk_psect_t	dg_sectors;	/* Sectors per track */
	dsk_psect_t	dg_secbase;	/* First physical sector number */
	size_t		dg_secsize;	/* Sector size in bytes */
	dsk_rate_t	dg_datarate;	/* Data rate */
	dsk_gap_t	dg_rwgap;	/* Read/write gap length */
	dsk_gap_t	dg_fmtgap;	/* Format gap length */
	int		dg_fm;		/* FM mode? The only thing I know that uses FM mode
					 * is the BBC Micro DFS format. */
	int		dg_nomulti;	/* Disable multitrack? */
} DSK_GEOMETRY;


/* Used when formatting a sector or reading its ID */
typedef struct
{
	dsk_pcyl_t	fmt_cylinder;
	dsk_phead_t	fmt_head;
	dsk_psect_t	fmt_sector;	
	size_t		fmt_secsize;
} DSK_FORMAT;

/***************************** GLOBAL FUNCTIONS ******************************/

/* Convert physical C/H/S to logical sector */
dsk_err_t dg_ps2ls(const DSK_GEOMETRY *self,  
	/* in */	dsk_pcyl_t cyl, dsk_phead_t head, dsk_psect_t sec,
	/* out */	dsk_lsect_t *logical);

/* Convert logical sector to physical C/H/S */
dsk_err_t dg_ls2ps(const DSK_GEOMETRY *self, 
	/* in */	dsk_lsect_t logical, 
	/* out */	dsk_pcyl_t *cyl, dsk_phead_t *head, dsk_psect_t *sec);

/* Convert physical cylinder/head to logical track */
dsk_err_t dg_pt2lt(const DSK_GEOMETRY *self,  
	/* in */	dsk_pcyl_t cyl, dsk_phead_t head,
	/* out */	dsk_ltrack_t *logical);

/* Convert logical track to physical cylinder/head */
dsk_err_t dg_lt2pt(const DSK_GEOMETRY *self, 
	/* in */	dsk_ltrack_t logical, 
	/* out */	dsk_pcyl_t *cyl, dsk_phead_t *head);
/* Expand error message */
char *dsk_strerror(/* in */ dsk_err_t err);

/* Initialise a DSK_GEOMETRY with one of the standard formats. 
 * If name / desc are not null, these are populated with the format's
 * short name and a brief description. */
dsk_err_t dg_stdformat(DSK_GEOMETRY *self, dsk_format_t formatid,
			dsk_cchar_t *name, dsk_cchar_t *desc);

/* Convert sector size to a physical sector shift as used by the controller.
 * To go the other way, size = 128 << psh  */
unsigned char dsk_get_psh(size_t sector_size);

/*****************************************************************************/

typedef struct
{
	int dr_forcehead;	/* Force drive to use head 0 or head 1? */
	struct drv_class *dr_class;
} DSK_DRIVER;


/* Open a DSK file, POSIX image, drive or whatever. 
 * "type" is:
 *   NULL    : Autodetect file type from name or magic number
 *   "dsk"   : CPCEMU DSK or EDSK format 
 *   "raw"   : Raw dd if=foo of=bar file
 *   "floppy": Host system's floppy drive 
 *   "myz80" : MYZ80 image file
 *
 * Will allocate a DSK_DRIVER object.
 */

dsk_err_t dsk_open(DSK_DRIVER **self, const char *filename, const char *type);

/* As for "open", but creates a new image file. On a floppy drive 
 * this will act exactly as "open"; for an image this will attempt to 
 * create the file new or truncate it.
 * Note that "type" cannot be NULL. */
dsk_err_t dsk_creat(DSK_DRIVER **self, const char *filename, const char *type);

/* Close a DSK file. Frees the pointer and sets it to null. */

dsk_err_t dsk_close(DSK_DRIVER **self);

/* Get drive status (Ready, Read-Only etc.). The actual status is 
 * based on the FDC's ST3 register. The following bits should be available: */

#define DSK_ST3_FAULT	0x80	/* Drive fault */
#define DSK_ST3_RO	0x40	/* Read only */
#define DSK_ST3_READY	0x20	/* Drive ready */
#define DSK_ST3_TRACK0	0x10	/* Head is over track 0 (not all drivers) */
#define DSK_ST3_DSDRIVE	0x08	/* Drive is double-sided */
#define DSK_ST3_HEAD1	0x04	/* Current head is head 1, not head 0 */

dsk_err_t dsk_drive_status(DSK_DRIVER *self, const DSK_GEOMETRY *geom, 
				dsk_phead_t head, unsigned char *status);

/* Read a sector. There are three alternative versions:
 *  One that uses physical sectors
 *  One that uses logical sectors
 *  One that uses physical sectors *which can have numbers not matching
 *  their positions on disc* - this functionality is only exposed by 
 *  drivers which can manipulate the FDC directly. */ 
dsk_err_t dsk_pread(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
                              void *buf, dsk_pcyl_t cylinder,
                              dsk_phead_t head, dsk_psect_t sector);
dsk_err_t dsk_lread(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
                              void *buf, dsk_lsect_t sector);
dsk_err_t dsk_xread(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
                              void *buf, 
			      dsk_pcyl_t cylinder, dsk_phead_t head, 
			      dsk_pcyl_t cyl_expected, dsk_phead_t head_expected,
			      dsk_psect_t sector, size_t sector_len);
/* Write a sector. There are three alternative versions:
 *  One that uses physical sectors
 *  One that uses logical sectors
 *  One that uses physical sectors *which can have numbers not matching
 *  their positions on disc* - this functionality is only exposed by 
 *  drivers which can manipulate the FDC directly. */ 
dsk_err_t dsk_pwrite(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
                              const void *buf, dsk_pcyl_t cylinder,
                              dsk_phead_t head, dsk_psect_t sector);
dsk_err_t dsk_lwrite(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
                              const void *buf, dsk_lsect_t sector);
dsk_err_t dsk_xwrite(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
                              const void *buf, 
			      dsk_pcyl_t cylinder, dsk_phead_t head, 
			      dsk_pcyl_t cyl_expected, dsk_phead_t head_expected,
			      dsk_psect_t sector, size_t sector_len);

/* Verify sector against memory buffer. There are three alternative versions:
 *  One that uses physical sectors
 *  One that uses logical sectors
 *  One that uses physical sectors *which can have numbers not matching
 *  their positions on disc* - this functionality is only exposed by 
 *  drivers which can manipulate the FDC directly. */ 
dsk_err_t dsk_pcheck(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
                              const void *buf, dsk_pcyl_t cylinder,
                              dsk_phead_t head, dsk_psect_t sector);
dsk_err_t dsk_lcheck(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
                              const void *buf, dsk_lsect_t sector);
dsk_err_t dsk_xcheck(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
                              const void *buf, 
			      dsk_pcyl_t cylinder, dsk_phead_t head, 
			      dsk_pcyl_t cyl_expected, dsk_phead_t head_expected,
			      dsk_psect_t sector, size_t sector_len);
/* Format a track.
 * Note that the geometry in these functions is not const; the CPCEMU driver
 * will increase the number of tracks/heads as the disc image file outgrows
 * the geometry.
 */
dsk_err_t dsk_pformat(DSK_DRIVER *self, DSK_GEOMETRY *geom,
				dsk_pcyl_t cylinder, dsk_phead_t head,
				const DSK_FORMAT *format, unsigned char filler);
dsk_err_t dsk_lformat(DSK_DRIVER *self, DSK_GEOMETRY *geom,
				dsk_ltrack_t track, const DSK_FORMAT *format, 
				unsigned char filler);
/* Read a track.
 */
dsk_err_t dsk_xtread(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
				void *buf, 
			        dsk_pcyl_t cylinder,     dsk_phead_t head,
				dsk_pcyl_t cyl_expected, dsk_phead_t head_expected);
dsk_err_t dsk_ptread(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
				void *buf, dsk_pcyl_t cylinder, 
				dsk_phead_t head);
dsk_err_t dsk_ltread(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
				void *buf, dsk_ltrack_t track);
/* Auto-format: generates the sector headers from "geom" */
dsk_err_t dsk_apform(DSK_DRIVER *self, DSK_GEOMETRY *geom,
				dsk_pcyl_t cylinder, dsk_phead_t head,
				unsigned char filler);
dsk_err_t dsk_alform(DSK_DRIVER *self, DSK_GEOMETRY *geom,
				dsk_ltrack_t track, unsigned char filler);

/* Probe the geometry of a disc. This will use the boot sector and any
 * information the driver can give. */
dsk_err_t dsk_getgeom(DSK_DRIVER *self, DSK_GEOMETRY *geom);
/* Convert various types of boot sector to DSK_GEOMETRY 
 * Return DSK_ERR_OK if successful, else DSK_ERR_BADFMT */
dsk_err_t dg_dosgeom(DSK_GEOMETRY *self, const unsigned char *bootsect);
dsk_err_t dg_pcwgeom(DSK_GEOMETRY *self, const unsigned char *bootsect);
dsk_err_t dg_cpm86geom(DSK_GEOMETRY *self, const unsigned char *bootsect);

/* Read a random sector header from current track */
dsk_err_t dsk_psecid(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
				dsk_pcyl_t cylinder, dsk_phead_t head,
				DSK_FORMAT *result);
dsk_err_t dsk_lsecid(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
				dsk_ltrack_t track, DSK_FORMAT *result);

/* Seek to a cylinder */
dsk_err_t dsk_lseek(DSK_DRIVER *self, const DSK_GEOMETRY *geom, 
				dsk_ltrack_t track);
dsk_err_t dsk_pseek(DSK_DRIVER *self, const DSK_GEOMETRY *geom, 
				dsk_pcyl_t cylinder, dsk_phead_t head);

/* If "index" is in range, returns the n'th driver name in (*drvname).
 * Else sets (*drvname) to null. */
dsk_err_t dsk_type_enum(int idx, char **drvname);

/* Force a drive to use head 0 or head 1 only for single-sided discs
 * Pass 0 or 1, or -1 to unset it. */
dsk_err_t dsk_set_forcehead(DSK_DRIVER *self, int force);
dsk_err_t dsk_get_forcehead(DSK_DRIVER *self, int *force);

/* Get the driver name and description */
const char *dsk_drvname(DSK_DRIVER *self);
const char *dsk_drvdesc(DSK_DRIVER *self);

/* Uncomment this to print on the console a trace of all mallocs */
#undef TRACE_MALLOCS 
#ifdef TRACE_MALLOCS
void *dsk_malloc(size_t size);
void  dsk_free(void *ptr);
#else
#define dsk_malloc malloc
#define dsk_free free
#endif

#ifdef __cplusplus
}
#endif

#endif /* LIBDSK_H */
