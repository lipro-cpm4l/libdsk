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

/* Access functions for compressed CPCEMU discs */

#include "drvi.h"
#include "drvdqk.h"

/* Unsqueeze engine functions */
static void init_cr(DQK_DSK_DRIVER *self);
static void init_dehuff(DQK_DSK_DRIVER *self);
static int getcr(DQK_DSK_DRIVER *self, FILE *ib);
static int getuhuff(DQK_DSK_DRIVER *self, FILE *ib);
static dsk_err_t getx16(DQK_DSK_DRIVER *self, FILE *iob, unsigned short *w);    
static dsk_err_t getw16(DQK_DSK_DRIVER *self, FILE *iob, signed short *w);    
static dsk_err_t unsqueeze(DQK_DSK_DRIVER *self, FILE *fp);
static dsk_err_t load_bytes(DQK_DSK_DRIVER *self, FILE *fp, unsigned char *data, size_t len);

/* Squeeze engine calls */
#define NOHIST	0 	/*don't consider previous input*/
#define SENTCHAR 1 	/*lastchar set, no lookahead yet */
#define SENDNEWC 2 	/*newchar set, previous sequence done */
#define SENDCNT 3 	/*newchar set, DLE sent, send count next */

/* *** Stuff for second translation module *** */

#define NOCHILD -1	/* indicates end of path through tree */
#define ERROR   -1

#define MAXCOUNT 0xFFFF	/* biggest unsigned integer */

static dsk_err_t squeeze(DQK_DSK_DRIVER *self, FILE *iob);
static void init_ncr(DQK_DSK_DRIVER *self);
static void init_huff(DQK_DSK_DRIVER *self);
static dsk_err_t wrt_head(DQK_DSK_DRIVER *self, FILE *ob);
static int gethuff(DQK_DSK_DRIVER *self);

static dsk_err_t putwe(DQK_DSK_DRIVER *self, int w,  FILE *iob);
static void init_tree(DQK_DSK_DRIVER *self);
static void init_enc(DQK_DSK_DRIVER *self);
static void bld_tree(DQK_DSK_DRIVER *self, int list[], int len);
static void adjust(DQK_DSK_DRIVER *self, int list[], int top, int bottom);
static void heap(DQK_DSK_DRIVER *self, int list[], int length);
static void scale(DQK_DSK_DRIVER *self, unsigned int ceil);
static int getcnr(DQK_DSK_DRIVER *self);
static int buildenc(DQK_DSK_DRIVER *self, int level, int root);
static char cmptrees(DQK_DSK_DRIVER *self, int a, int b);


/* This struct contains function pointers to the driver's functions, and the
 * size of its DSK_DRIVER subclass */

DRV_CLASS dc_dqk = 
{
	sizeof(DQK_DSK_DRIVER),
	"dqk",
	"DQK (compressed .DSK) driver",
	dqk_open,	/* open */
	dqk_creat,	/* create new */
	dqk_close,	/* close */
	dqk_read,	/* read sector, working from physical address */
	dqk_write,	/* write sector, working from physical address */
	dqk_format,	/* format track, physical */
	NULL,		/* get geometry */
	dqk_secid,	/* logical sector ID */
	dqk_xseek,	/* seek to track */
	dqk_status,	/* get drive status */
	dqk_xread,	/* read sector */
	dqk_xwrite,	/* write sector */ 
};

static void dqk_free(DQK_DSK_DRIVER *self)
{
	int n;

	for (n = 0; n < self->dqk_ntracks; n++)
	{
		DQK_TRACK *t = &self->dqk_tracks[n];

		if (t->dqkt_data) free(t->dqkt_data);
	}
	free(self->dqk_tracks);
	self->dqk_tracks = NULL;
	self->dqk_ntracks = 0;

        if (self->dqk_filename)
        {
                free(self->dqk_filename);
                self->dqk_filename = NULL;
        }
        if (self->dqk_truename)
        {
                free(self->dqk_filename);
                self->dqk_filename = NULL;
        }
}

static dsk_err_t dqk_ensure_size(DQK_DSK_DRIVER *self, dsk_ltrack_t trk)
{
        /* Ensure the requested track is in range, by starting at 
         *          * max (dqk_ntracks, 1) and doubling repeatedly */
        dsk_ltrack_t maxtrks = self->dqk_ntracks;

        if (!maxtrks) maxtrks = 1;
        while (trk >= maxtrks) maxtrks *= 2;

        /* Need to allocate more. */
        if (maxtrks != self->dqk_ntracks)
        {
                /* Create new array of tracks */
                DQK_TRACK *t = malloc(maxtrks * sizeof(DQK_TRACK));
                if (!t) return DSK_ERR_NOMEM;
                memset(t, 0, maxtrks * sizeof(DQK_TRACK));
                /* Copy over existing array */
                memcpy(t, self->dqk_tracks, self->dqk_ntracks * sizeof(DQK_TRACK));
                free(self->dqk_tracks);
                self->dqk_tracks = t;
                self->dqk_ntracks = maxtrks;
        }
        return DSK_ERR_OK;
}


/* Open DSK image, checking for the magic number */
dsk_err_t dqk_open(DSK_DRIVER *self, const char *filename)
{
	DQK_DSK_DRIVER *dqk_self;
	FILE *fp;
	int err;
	
	/* Sanity check: Is this meant for our driver? */
	if (self->dr_class != &dc_dqk) return DSK_ERR_BADPTR;
	dqk_self = (DQK_DSK_DRIVER *)self;

	fp = fopen(filename, "r+b");
	if (!fp) 
	{
		dqk_self->dqk_readonly = 1;
		fp = fopen(filename, "rb");
	}
	if (!fp) return DSK_ERR_NOTME;

	dqk_self->dqk_dirty = 0;
        /* Keep a copy of the filename; when writing back, we will need it */
	dqk_self->dqk_filename = malloc(1 + strlen(filename));
	if (!dqk_self->dqk_filename) return DSK_ERR_NOMEM;
	strcpy(dqk_self->dqk_filename, filename);

	/* Load the file */
	err = unsqueeze(dqk_self, fp);
	fclose(fp);
	if (err) return err;	

	dqk_self->dqk_trkhead[0] = 0;
	return DSK_ERR_OK;
}

/* Create DSK image */
dsk_err_t dqk_creat(DSK_DRIVER *self, const char *filename)
{
	DQK_DSK_DRIVER *dqk_self;
	FILE *fp;
	char *ss;
	
	/* Sanity check: Is this meant for our driver? */
	if (self->dr_class != &dc_dqk) return DSK_ERR_BADPTR;
	dqk_self = (DQK_DSK_DRIVER *)self;

	fp = fopen(filename, "wb");
	dqk_self->dqk_readonly = 0;
	if (!fp) return DSK_ERR_SYSERR;
	fclose(fp);

	/* Create a default header */
        memset(dqk_self->dqk_dskhead, 0, 256);
        strcpy((char *)dqk_self->dqk_dskhead,
		"MV - CPCEMU Disk-File\r\nDisk-Info\r\n(LIBDSK)");
	dqk_self->dqk_trkhead[0] = 0;
        dqk_self->dqk_dirty = 1;

        /* Keep a copy of the filename, for writing back */
        dqk_self->dqk_filename = malloc(1 + strlen(filename));
        if (!dqk_self->dqk_filename) return DSK_ERR_NOMEM;
        strcpy(dqk_self->dqk_filename, filename);

	/* True name can't really be guessed, but we'll give it a go... */
        dqk_self->dqk_truename = malloc(1 + strlen(filename));
        if (!dqk_self->dqk_truename) 
	{
		free(dqk_self->dqk_filename);
		return DSK_ERR_NOMEM;
	}
	/* UNIX SQ squeezes files by appending ".SQ" */
        strcpy(dqk_self->dqk_truename, filename);
	ss = strstr(dqk_self->dqk_truename, ".SQ");
	if (ss) *ss = 0;

	/* Convert .DQK back to .DSK */
	ss = strstr(dqk_self->dqk_truename, ".DQK");
	if (ss) memcpy(ss, ".DSK", 4);

	/* Convert .DQK back to .DSK */
	ss = strstr(dqk_self->dqk_truename, ".dqk");
	if (ss) memcpy(ss, ".dsk", 4);

        dqk_self->dqk_ntracks = 0;
        dqk_self->dqk_tracks = NULL;

        return DSK_ERR_OK;
}


dsk_err_t dqk_close(DSK_DRIVER *self)
{
	DQK_DSK_DRIVER *dqk_self;
	dsk_err_t err = DSK_ERR_OK;
	
	if (self->dr_class != &dc_dqk) return DSK_ERR_BADPTR;
	dqk_self = (DQK_DSK_DRIVER *)self;

        if (dqk_self->dqk_filename && (dqk_self->dqk_dirty))
        {
		FILE *fp = fopen(dqk_self->dqk_filename, "wb");
		if (!fp) err = DSK_ERR_SYSERR;
		else
                {
			err = squeeze(dqk_self, fp);
			fclose(fp);
		}
	}
	dqk_free(dqk_self);

	return err;
}






/* Find the track corresponding to a particular cylinder/head. 
 *
 * DSK files work in "tracks". For a single-sided disk, track number
 * is the same as cylinder number. For a double-sided disk, track number is
 * (2 * cylinder + head). This is independent of disc format.
 */
static DQK_TRACK *lookup_track(DQK_DSK_DRIVER *self, 
			const DSK_GEOMETRY *geom,
			  dsk_pcyl_t cylinder, dsk_phead_t head)
{
	dsk_ltrack_t track;
	DQK_TRACK *t;
	
	/* Convert cylinder & head to DQK "track" */
	track = cylinder;
	if (self->dqk_dskhead[0x31] > 1) track *= 2;
	track += head;

	dqk_ensure_size(self, track);
	t = &(self->dqk_tracks[track]);
	return t;
}





/* Seek to a cylinder. Checks if that particular cylinder exists. 
 * We test for the existence of a cylinder by looking for Track <n>, Head 0.
 * Fortunately the DSK format does not allow for discs with different numbers
 * of tracks on each side (though this is obviously possible with a real disc)
 * so if head 0 exists then the whole cylinder does. 


static dsk_err_t seek_cylinder(DQK_DSK_DRIVER *self, DSK_GEOMETRY *geom, int cylinder)
{
	long nr;
	if (!self->dqk_fp) return DSK_ERR_NOTRDY;

	// Check if the DSK image goes out to the correct cylinder 
	nr = lookup_track(self, geom, cylinder, 0);
	
	if (nr < 0) return DSK_ERR_SEEKFAIL;
	return DSK_ERR_OK;
}
*/

/* Load the "Track-Info" header for the given cylinder and head */
static dsk_err_t load_track_header(DQK_DSK_DRIVER *self, const DSK_GEOMETRY *geom, 
					int cylinder, int head)
{
        DQK_TRACK *track;

	track = lookup_track(self, geom, cylinder, head);
        if (!track) return DSK_ERR_SEEKFAIL;       /* Bad track */

	if (track->dqkt_length < 256) return DSK_ERR_NOADDR;	/* No track header */

	memcpy(self->dqk_trkhead, track->dqkt_data, 256);
	return DSK_ERR_OK;
}


/* Read a sector ID from a given track */
dsk_err_t dqk_secid(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
                                dsk_pcyl_t cyl, dsk_phead_t head, DSK_FORMAT *result)
{
	DQK_DSK_DRIVER *dqk_self;
	dsk_err_t e;
	int offs;

	if (!self || !geom || !result || self->dr_class != &dc_dqk) 
		return DSK_ERR_BADPTR;
	dqk_self = (DQK_DSK_DRIVER *)self;
	
	if (!dqk_self->dqk_filename) return DSK_ERR_NOTRDY;

	e = load_track_header(dqk_self, geom, cyl, head);
	if (e) return e;

	/* Offset of the chosen sector header */
	++dqk_self->dqk_sector;
	offs = 0x18 + 8 * (dqk_self->dqk_sector % dqk_self->dqk_trkhead[0x15]);

	result->fmt_cylinder = dqk_self->dqk_trkhead[offs];
	result->fmt_head     = dqk_self->dqk_trkhead[offs+1];
	result->fmt_sector   = dqk_self->dqk_trkhead[offs+2];
	result->fmt_secsize  = 128 << dqk_self->dqk_trkhead[offs+3];
	return DSK_ERR_OK;	
}


/* Find the offset of a sector in the current track 
 * Enter with dqk_trkhead loaded and the file pointer 
 * just after it (ie, you have just called load_track_header() ) */

static long sector_offset(DQK_DSK_DRIVER *self, dsk_psect_t sector, size_t *seclen,
			      unsigned char **secid)
{
	int maxsec = self->dqk_trkhead[0x15];
	long offset = 0;
	int n;

	/* Pointer to sector details */
	*secid = self->dqk_trkhead + 0x18;

	/* Length of sector */	
	*seclen = (0x80 << self->dqk_trkhead[0x14]);

	/* Extended DSKs have individual sector sizes */
	if (!memcmp(self->dqk_dskhead, "EXTENDED", 8))
	{
		for (n = 0; n < maxsec; n++)
		{
			*seclen = (*secid)[7] + 256 * (*secid)[8];
                       if ((*secid)[2] == sector) return offset;
			offset   += (*seclen);
			(*secid) += 8;
		}
	}
	else	/* Non-extended, all sector sizes are the same */
	{
		for (n = 0; n < maxsec; n++)
		{
			if ((*secid)[2] == sector) return offset;
			offset   += (*seclen);
			(*secid) += 8;
		}
	}
	return -1;	/* Sector not found */
}



/* Seek within the DSK file to a given head & sector in the current cylinder. */
static dsk_err_t seekto_sector(DQK_DSK_DRIVER *self, const DSK_GEOMETRY *geom,
		int cylinder, int head, int cyl_expected,
		int head_expected, int sector, size_t *len, 
		unsigned char **data)
{
        size_t seclen;
	dsk_err_t err;
	unsigned char *secid;
	DQK_TRACK *dqkt;
	dsk_ltrack_t track;
	int offs;

	track = cylinder;
	if (self->dqk_dskhead[0x31] > 1) track *= 2;
	track += head;

        err = load_track_header(self, geom, cylinder, head);
        if (err) return err;

	offs = sector_offset(self, sector, &seclen, &secid);
	if (offs < 0) return DSK_ERR_NOADDR;	/* Sector not found */
	offs += 256;	/* Account for header */

	if (cyl_expected != secid[0] || head_expected != secid[1])
	{
		/* We are not in the right place */
		return DSK_ERR_NOADDR;
	}
	if (seclen < (*len))
	{
		err = DSK_ERR_DATAERR;
		*len = seclen;
	}
	else if (seclen > (*len))
	{
		err = DSK_ERR_DATAERR;
		seclen = *len;
	}

	if (track >= self->dqk_ntracks) return DSK_ERR_NOADDR;
	dqkt = &self->dqk_tracks[track];
	if (!dqkt->dqkt_data || (dqkt->dqkt_length < offs))
	{
		return DSK_ERR_NOADDR;
	}

	*data = dqkt->dqkt_data + offs;

	return err;
}


/* Read a sector */
dsk_err_t dqk_read(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
                             void *buf, dsk_pcyl_t cylinder,
                              dsk_phead_t head, dsk_psect_t sector)
{
	return dqk_xread(self, geom, buf, cylinder, head, cylinder,
			    head, sector, geom->dg_secsize);
}

dsk_err_t dqk_xread(DSK_DRIVER *self, const DSK_GEOMETRY *geom, void *buf, 
			      dsk_pcyl_t cylinder,   dsk_phead_t head, 
			      dsk_pcyl_t cyl_expect, dsk_phead_t head_expect,
			      dsk_psect_t sector, size_t sector_size)
{
	DQK_DSK_DRIVER *dqk_self;
	dsk_err_t err;
	size_t len = geom->dg_secsize;
	unsigned char *data;

	if (!buf || !geom || !self) return DSK_ERR_BADPTR;
	if (self->dr_class != &dc_dqk) return DSK_ERR_BADPTR;
	dqk_self = (DQK_DSK_DRIVER *)self;

	err  = seekto_sector(dqk_self, geom, cylinder,head, 
					cyl_expect, head_expect, sector, &len, &data);
	if (err == DSK_ERR_DATAERR || err == DSK_ERR_OK)
	{
		if (len > sector_size) len = sector_size;
		memcpy(buf, data, len);
	}
	return err;
}		


/* Write a sector */
dsk_err_t dqk_write(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
                             const void *buf, dsk_pcyl_t cylinder,
                              dsk_phead_t head, dsk_psect_t sector)
{
	return dqk_xwrite(self, geom, buf, cylinder, head, cylinder,head,
				sector, geom->dg_secsize);
}

dsk_err_t dqk_xwrite(DSK_DRIVER *self, const DSK_GEOMETRY *geom, 
			      const void *buf, 
			      dsk_pcyl_t cylinder,   dsk_phead_t head, 
			      dsk_pcyl_t cyl_expect, dsk_phead_t head_expect,
			      dsk_psect_t sector, size_t sector_size)
{
	DQK_DSK_DRIVER *dqk_self;
	dsk_err_t err;
	size_t len = geom->dg_secsize;
	unsigned char *data;

	if (!buf || !geom || !self) return DSK_ERR_BADPTR;
	if (self->dr_class != &dc_dqk) return DSK_ERR_BADPTR;
	dqk_self = (DQK_DSK_DRIVER *)self;

	if (dqk_self->dqk_readonly) return DSK_ERR_RDONLY;

	err  = seekto_sector(dqk_self, geom, cylinder,head,  
				cyl_expect, head_expect, sector, &len, &data);
	if (err == DSK_ERR_DATAERR || err == DSK_ERR_OK)
	{
		if (len > sector_size) len = sector_size;
		memcpy(data, buf, len);
        }
	return err;
}



/* Format a track on a DSK. Can grow the DSK file. */
dsk_err_t dqk_format(DSK_DRIVER *self, DSK_GEOMETRY *geom,
                                dsk_pcyl_t cylinder, dsk_phead_t head,
                                const DSK_FORMAT *format, unsigned char filler)
{
	DQK_DSK_DRIVER *dqk_self;
	int n, trklen, trkno, ext, seclen;
	DQK_TRACK *dqkt;
	unsigned char *data;

	if (!format || !geom || !self) return DSK_ERR_BADPTR;
	if (self->dr_class != &dc_dqk) return DSK_ERR_BADPTR;
	dqk_self = (DQK_DSK_DRIVER *)self;

	if (!dqk_self->dqk_filename) return DSK_ERR_NOTRDY;
	if (dqk_self->dqk_readonly)  return DSK_ERR_RDONLY;

	ext = 0;

/* 1. Only if the DSK has either (1 track & 1 head) or (2 heads) can we
 *   format the second head
 */
	if (head)
	{
		if (dqk_self->dqk_dskhead[0x31] == 1 && 
		    dqk_self->dqk_dskhead[0x30] > 1) return DSK_ERR_RDONLY;

		if (dqk_self->dqk_dskhead[0x31] == 1) 
			dqk_self->dqk_dskhead[0x31] = 2;
	}
/* 2. Find out the track number of the new cylinder/head */

	if (dqk_self->dqk_dskhead[0x31] < 1) dqk_self->dqk_dskhead[0x31] = 1;
	trkno = cylinder;
	trkno *= dqk_self->dqk_dskhead[0x31];
	trkno += head;

/* 3. Find out how long the proposed new track is
 *
 * nb: All sizes *include* track header
 */
	trklen = 0;
	for (n = 0; n < geom->dg_sectors; n++)
	{
		trklen += format[n].fmt_secsize;
	}
	trklen += 256;	/* For header */

/* Seek to the track. */
	dqkt = lookup_track(dqk_self, geom, cylinder, head);
	if (!dqkt) return DSK_ERR_RDONLY;

	if (dqkt->dqkt_data) free(dqkt->dqkt_data);
	dqkt->dqkt_length = 0;
	dqkt->dqkt_data = malloc(trklen);
	if (!dqkt->dqkt_data) return DSK_ERR_NOMEM;
	dqkt->dqkt_length = trklen;

	/* Now generate and write a Track-Info buffer */
	memset(dqk_self->dqk_trkhead, 0, sizeof(dqk_self->dqk_trkhead));

	strcpy((char *)dqk_self->dqk_trkhead, "Track-Info\r\n");

	dqk_self->dqk_trkhead[0x10] = (unsigned char)cylinder;
	dqk_self->dqk_trkhead[0x11] = (unsigned char)head;
	dqk_self->dqk_trkhead[0x14] = dsk_get_psh(format[0].fmt_secsize);
	dqk_self->dqk_trkhead[0x15] = (unsigned char)geom->dg_sectors;
	dqk_self->dqk_trkhead[0x16] = geom->dg_fmtgap;
	dqk_self->dqk_trkhead[0x17] = filler;
	for (n = 0; n < geom->dg_sectors; n++)
	{
		dqk_self->dqk_trkhead[0x18 + 8*n] = (unsigned char)format[n].fmt_cylinder;
		dqk_self->dqk_trkhead[0x19 + 8*n] = (unsigned char)format[n].fmt_head;
		dqk_self->dqk_trkhead[0x1A + 8*n] = (unsigned char)format[n].fmt_sector;
		dqk_self->dqk_trkhead[0x1B + 8*n] = dsk_get_psh(format[n].fmt_secsize);
		if (ext)
		{
			seclen = format[n].fmt_secsize;
			dqk_self->dqk_trkhead[0x1E + 8 * n] = (unsigned char)(seclen & 0xFF);
			dqk_self->dqk_trkhead[0x1F + 8 * n] = (unsigned char)(seclen >> 8);
		}
	}
	memcpy(dqkt->dqkt_data, dqk_self->dqk_trkhead, 256);
	data = dqkt->dqkt_data + 256;

	/* Track header written. Write sectors */
	for (n = 0; n < geom->dg_sectors; n++)
	{
		seclen = format[n].fmt_secsize;
		memset(data, filler ,seclen);
		data += seclen;
	}

	if (cylinder >= dqk_self->dqk_dskhead[0x30])
	{
		dqk_self->dqk_dskhead[0x30] = (unsigned char)(cylinder + 1);
	}

	if (!memcmp(dqk_self->dqk_dskhead, "EXTENDED", 8))
	{
		dqk_self->dqk_dskhead[0x34 + trkno] = trklen >> 8;
	}
	else
	{
		size_t otrklen = dqk_self->dqk_dskhead[0x32] + 256 * 
			         dqk_self->dqk_dskhead[0x33];
	
		if (trklen > otrklen)
		{
			dqk_self->dqk_dskhead[0x32] = trklen * 0xFF;
			dqk_self->dqk_dskhead[0x33] = trklen >> 8;
		}
	}

	dqk_self->dqk_dirty = 1;

	/* Track formatted OK. */
	
	/* If the disc image has grown because of this, record this in the
	 * disc geometry struct */
	
	if (geom->dg_heads     < dqk_self->dqk_dskhead[0x31]) 
		geom->dg_heads     = dqk_self->dqk_dskhead[0x31];
	if (geom->dg_cylinders < dqk_self->dqk_dskhead[0x30])
		geom->dg_cylinders = dqk_self->dqk_dskhead[0x30]; 
		
	return DSK_ERR_OK;
}



/* Seek to a cylinder. */
dsk_err_t dqk_xseek(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
                                dsk_pcyl_t cyl, dsk_phead_t head)
{
	DQK_DSK_DRIVER *dqk_self;

	if (!self || !geom || self->dr_class != &dc_dqk) 
		return DSK_ERR_BADPTR;
	dqk_self = (DQK_DSK_DRIVER *)self;
	
	if (!dqk_self->dqk_filename) return DSK_ERR_NOTRDY;

	// See if the cylinder & head are in range
	if (cyl  > dqk_self->dqk_dskhead[0x30] ||
            head > dqk_self->dqk_dskhead[0x31]) return DSK_ERR_SEEKFAIL;
	return DSK_ERR_OK;
}


dsk_err_t dqk_status(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
                      dsk_phead_t head, unsigned char *result)
{
        DQK_DSK_DRIVER *dqk_self;

        if (!self || !geom || self->dr_class != &dc_dqk) return DSK_ERR_BADPTR;
        dqk_self = (DQK_DSK_DRIVER *)self;

        if (!dqk_self->dqk_filename) *result &= ~DSK_ST3_READY;
        if (dqk_self->dqk_readonly) *result |= DSK_ST3_RO;
	return DSK_ERR_OK;
}



dsk_err_t unsqueeze(DQK_DSK_DRIVER *self, FILE *inbuff)
{
	int i, c;

	unsigned short filecrc;	/* checksum */
	unsigned short numnodes;	/* size of decoding tree */
	int err;
	long lpos1, lpos2;
	dsk_ltrack_t track, tracks;
	size_t tracklen;
	unsigned short magic;

	/* Initialization */
	self->crc = 0;
	init_cr(self);
	init_dehuff(self);

	/* Process header */
	err = getx16(self, inbuff, &magic);
	if (err) return err;
	if(magic != RECOGNIZE) return DSK_ERR_NOTME;

	err = getx16(self, inbuff, &filecrc);
	if (err) return err;
	/* Get original file name */

	lpos1 = ftell(inbuff);
	do
	{
		c = fgetc(inbuff);
		if (c == EOF) return DSK_ERR_NOTME;
	}
	while (c != 0);

	lpos2 = ftell(inbuff);
	self->dqk_truename = malloc(lpos2 - lpos1);
	if (!self->dqk_truename) return DSK_ERR_NOMEM;
	fseek(inbuff, lpos1, SEEK_SET);
	fread(self->dqk_truename, 1, lpos2 - lpos1, inbuff);

	err = getx16(self, inbuff, &numnodes);
	if (err) return err;
	
	if(numnodes >= NUMVALS) 
	{
		/* Invalid decode tree size */
		return DSK_ERR_NOTME;
	}

	/* Initialize for possible empty tree (SPEOF only) */
	self->dnode[0].children[0] = -(SPEOF + 1);
	self->dnode[0].children[1] = -(SPEOF + 1);

	/* Get decoding tree from file */
	for(i = 0; i < numnodes; ++i) 
	{
		err = getw16(self, inbuff, &self->dnode[i].children[0]);
		if (err) return err;
		err = getw16(self, inbuff, &self->dnode[i].children[1]);
		if (err) return err;
	}

	/* Load the DSK header */
	err = load_bytes(self, inbuff, self->dqk_dskhead, 256); if (err) return err;

	/* Not a DSK header? */
        if (memcmp("MV - CPC", self->dqk_dskhead, 8) &&
            memcmp("EXTENDED", self->dqk_dskhead, 8)) return DSK_ERR_NOTME;

	tracks = self->dqk_dskhead[0x30] * self->dqk_dskhead[0x31];
	/* Load tracks */
	for (track = 0; track < tracks; track++)
	{
		DQK_TRACK *trk;

		if (!memcmp(self->dqk_dskhead, "EXTENDED", 8))
		{
			tracklen = self->dqk_dskhead[0x34 + track];
			tracklen *= 256;	
		}
		else
		{
			tracklen  = self->dqk_dskhead[0x33] * 256;
			tracklen += self->dqk_dskhead[0x32];
		}
		/* printf("track %d len=%x\n", track, tracklen); */
		err = load_bytes(self, inbuff, self->dqk_trkhead, 256); 
		if (err) break;
		/* printf("got header\n", track, tracklen); */
        	if (memcmp(self->dqk_trkhead, "Track-Info", 10)) 
		{
			err = DSK_ERR_NOTME;
			break;
		}
		/* printf("valid header\n", track, tracklen); */
		err = dqk_ensure_size(self, track);
		if (err) break;
		/* printf("ensured size\n", track, tracklen); */

		/* Allocate space for the track */
		trk = &self->dqk_tracks[track];
		if (trk->dqkt_data) free(trk->dqkt_data);
		trk->dqkt_length = tracklen;
		trk->dqkt_data = malloc(trk->dqkt_length);
		if (!trk->dqkt_data) { err = DSK_ERR_NOMEM; break; }
		/* printf("allocated %x bytes\n", tracklen); */
		memcpy(trk->dqkt_data, self->dqk_trkhead, 256);
		err = load_bytes(self, inbuff, trk->dqkt_data + 256, tracklen - 256);
		if (err) break; 
		/* printf("loaded %x bytes\n", tracklen); */
	}
	if (!err)
	{	/* Checksum error */
		if((filecrc && 0xFFFF) != (self->crc && 0xFFFF))
			return DSK_ERR_NOTME;
	}
	if (err) dqk_free(self);
	return err;
}

static dsk_err_t load_bytes(DQK_DSK_DRIVER *self, FILE *fp, unsigned char *data, size_t len)
{
	int c;
	int off = 0;

	while((c = getcr(self, fp)) != EOF) 
	{
		self->crc += c;
		data[off++] = c;
		if (off >= len) return DSK_ERR_OK;
	}
	return DSK_ERR_NOTME;	/* Unexpected EOF */
}


/* get 16-bit unsigned word from file */
dsk_err_t getx16(DQK_DSK_DRIVER *self, FILE *iob, unsigned short *v)
{
	int temp;
	unsigned short rv;

	temp = fgetc(iob);		/* get low order byte */
	if (temp == EOF) return DSK_ERR_NOTME;
	rv = temp;
	temp = fgetc(iob);		/* get high order byte */
	if (temp == EOF) return DSK_ERR_NOTME;
	rv |= (temp << 8);
	*v = rv;
	return DSK_ERR_OK;
}

/* get 16-bit signed word from file */
dsk_err_t getw16(DQK_DSK_DRIVER *self, FILE *iob, signed short *v)
{
	unsigned short u;
	dsk_err_t err = getx16(self, iob, &u);
	if (err) return err;

	if (u & 0x8000)
	{
		int i = (u & 0x7FFF) - 0x8000;
		*v = i;
	}
	else *v = u;
	return DSK_ERR_OK;

}



void init_cr(DQK_DSK_DRIVER *self)
{
	self->repct = 0;
}

void init_dehuff(DQK_DSK_DRIVER *self)
{
	self->bpos = 99;	/* force initial read */
}

/* Get bytes with decoding - this decodes repetition,
 * calls getuhuff to decode file stream into byte
 * level code with only repetition encoding.
 *
 * The code is simple passing through of bytes except
 * that DLE is encoded as DLE-zero and other values
 * repeated more than twice are encoded as value-DLE-count.
 */

int getcr(DQK_DSK_DRIVER *self, FILE *ib)
{
	int c;

	if(self->repct > 0) {
		/* Expanding a repeated char */
		--self->repct;
		return self->value;
	} else {
		/* Nothing unusual */
		if((c = getuhuff(self, ib)) != DLE) {
			/* It's not the special delimiter */
			self->value = c;
			if(self->value == EOF)
				self->repct = LARGE;
			return self->value;
		} else {
			/* Special token */
			if((self->repct = getuhuff(self, ib)) == 0)
				/* DLE, zero represents DLE */
				return DLE;
			else {
				/* Begin expanding repetition */
				self->repct -= 2;	/* 2nd time */
				return self->value;
			}
		}
	}
	return EOF;
}

/* ejecteject */

/* Decode file stream into a byte level code with only
 * repetition encoding remaining.
 */

int getuhuff(DQK_DSK_DRIVER *self, FILE *ib)
{
	int i;

	/* Follow bit stream in tree to a leaf*/
	i = 0;	/* Start at root of tree */
	do {
		if(++self->bpos > 7) {
			if((self->curin = fgetc(ib)) == EOF)
				return EOF;
			self->bpos = 0;
			/* move a level deeper in tree */
			i = self->dnode[i].children[1 & self->curin];
		} else
			i = self->dnode[i].children[1 & (self->curin >>= 1)];
	} while(i >= 0);

	/* Decode fake node index to original data value */
	i = -(i + 1);
	/* Decode special endfile token to normal EOF */
	i = (i == SPEOF) ? EOF : i;
	return i;
}


// XXX Lots of error checking to do
static dsk_err_t squeeze(DQK_DSK_DRIVER *self, FILE *iob)
{
	int c;
	dsk_err_t err;

	self->crc = 0;
	/* First pass: Analyze */
	init_ncr(self);
	init_huff(self);
	/* Write output header */	
	err = wrt_head(self, iob); 
	if (err) return err;

	/* Second pass: encode the file */

	init_ncr(self);	/* For second pass */

	/* Translate the input file into the output file */
	while((c = gethuff(self)) != EOF)
	{
		err = (fputc(c, iob) == EOF);
		if (err) return DSK_ERR_SYSERR; 
	}
	return DSK_ERR_OK;
}


static int getc_crc(DQK_DSK_DRIVER *self)
{
	DQK_TRACK *trk;
	unsigned char ch;

	while (1)	
	{
		/* DSK header */
		if (self->enc_track == -1)
		{
			if (self->enc_offs > 255)
			{
				self->enc_offs = 0;
				++self->enc_track;
				continue;
			}
			ch = self->dqk_dskhead[self->enc_offs++];
			self->crc += ch;
			break;
		}

		/* DSK body */
		if (self->enc_track >= self->dqk_ntracks) return EOF;

		trk = &(self->dqk_tracks[self->enc_track]);

		if (trk->dqkt_data == NULL) 
		{
			++self->enc_track;	
			continue;
		}
		if (self->enc_offs >= trk->dqkt_length)
		{
			++self->enc_track;
			self->enc_offs = 0;
			continue;
		}
		ch = trk->dqkt_data[self->enc_offs++];
		self->crc += ch;
		break;
	}
	return ch;
}



void init_ncr(DQK_DSK_DRIVER *self)	/*initialize getcnr() */
{
	self->state = NOHIST;
	self->enc_track = -1;
	self->enc_offs  = 0;
}

int getcnr(DQK_DSK_DRIVER *self)
{
	switch(self->state) 
	{
	case NOHIST:
		/* No relevant history */
		self->state = SENTCHAR;
		return self->lastchar = getc_crc(self);   
	case SENTCHAR:
		/* Lastchar is set, need lookahead */
		switch(self->lastchar) 
		{
		case DLE:
			self->state = NOHIST;
			return 0;	/* indicates DLE was the data */
		case EOF:
			return EOF;
		default:
			for(self->likect = 1; (self->newchar = getc_crc(self)) == self->lastchar && self->likect < 255; ++self->likect)
				;
			switch(self->likect) {
			case 1:
				return self->lastchar = self->newchar;
			case 2:
				/* just pass through */
				self->state = SENDNEWC;
				return self->lastchar;
			default:
				self->state = SENDCNT;
				return DLE;
			}
		}
	case SENDNEWC:
		/* Previous sequence complete, newchar set */
		self->state = SENTCHAR;
		return self->lastchar = self->newchar;
	case SENDCNT:
		/* Sent DLE for repeat sequence, send count */
		self->state = SENDNEWC;
		return self->likect;
	default:
		fputs("LIBDSK internal error: SQ Bug - bad state\n", stderr);
		exit(1);
	}
}


/******** Second translation - bytes to variable length bit strings *********/


/* This translation uses the Huffman algorithm to develop a
 * binary tree representing the decoding information for
 * a variable length bit string code for each input value.
 * Each string's length is in inverse proportion to its
 * frequency of appearance in the incoming data stream.
 * The encoding table is derived from the decoding table.
 *
 * The range of valid values into the Huffman algorithm are
 * the values of a byte stored in an integer plus the special
 * endfile value chosen to be an adjacent value. Overall, 0-SPEOF.
 *
 * The "node" array of structures contains the nodes of the
 * binary tree. The first NUMVALS nodes are the leaves of the
 * tree and represent the values of the data bytes being
 * encoded and the special endfile, SPEOF.
 * The remaining nodes become the internal nodes of the tree.
 *
 * In the original design it was believed that
 * a Huffman code would fit in the same number of
 * bits that will hold the sum of all the counts.
 * That was disproven by a user's file and was a rare but
 * infamous bug. This version attempts to choose among equally
 * weighted subtrees according to their maximum depths to avoid
 * unnecessarily long codes. In case that is not sufficient
 * to guarantee codes <= 16 bits long, we initially scale
 * the counts so the total fits in an unsigned integer, but
 * if codes longer than 16 bits are generated the counts are
 * rescaled to a lower ceiling and code generation is retried.
 */

/* Initialize the Huffman translation. This requires reading
 * the input file through any preceding translation functions
 * to get the frequency distribution of the various values.
 */

void init_huff(DQK_DSK_DRIVER *self)          
{
	int c, i;
	int btlist[NUMVALS];	/* list of intermediate binary trees */
	int listlen;		/* length of btlist */
	unsigned int *wp;	/* simplifies weight counting */
	unsigned int ceiling;	/* limit for scaling */

	/* Initialize tree nodes to no weight, no children */
	init_tree(self);

	/* Build frequency info in tree */
	do {
		c = getcnr(self);        
		if(c == EOF)
			c = SPEOF;
		if(*(wp = &self->node[c].weight) !=  MAXCOUNT)
			++(*wp);
	} while(c != SPEOF);
	ceiling = MAXCOUNT;

	do {	/* Keep trying to scale and encode */
		scale(self, ceiling);
		ceiling /= 2;	/* in case we rescale */

		/* Build list of single node binary trees having
		 * leaves for the input values with non-zero counts
		 */
		for(i = listlen = 0; i < NUMVALS; ++i)
			if(self->node[i].weight != 0) {
				self->node[i].tdepth = 0;
				btlist[listlen++] = i;
			}

		/* Arrange list of trees into a heap with the entry
		 * indexing the node with the least weight at the top.
		 */
		heap(self, btlist, listlen);

		/* Convert the list of trees to a single decoding tree */
		bld_tree(self, btlist, listlen);

		/* Initialize the encoding table */
		init_enc(self);

		/* Try to build encoding table.
		 * Fail if any code is > 16 bits long.
		 */
	} while(buildenc(self, 0, self->dctreehd) == ERROR);

	/* Initialize encoding variables */
	self->cbitsrem = 0;	/*force initial read */
	self->curin = 0;	/*anything but endfile*/
}
/*  */
/* The count of number of occurrances of each input value
 * have already been prevented from exceeding MAXCOUNT.
 * Now we must scale them so that their sum doesn't exceed
 * ceiling and yet no non-zero count can become zero.
 * This scaling prevents errors in the weights of the
 * interior nodes of the Huffman tree and also ensures that
 * the codes will fit in an unsigned integer. Rescaling is
 * used if necessary to limit the code length.
 */

void scale(DQK_DSK_DRIVER *self, unsigned int ceil)
{
	int ovflw, divisor, i;
	unsigned int w, sum;
	char increased;		/* flag */

	do {
		for(i = sum = ovflw = 0; i < NUMVALS; ++i) {
			if(self->node[i].weight > (ceil - sum))
				++ovflw;
			sum += self->node[i].weight;
		}

		divisor = ovflw + 1;

		/* Ensure no non-zero values are lost */
		increased = 0;
		for(i = 0; i < NUMVALS; ++i) {
			w = self->node[i].weight;
			if (w < divisor && w > 0) {
				/* Don't fail to provide a code if it's used at all */
				self->node[i].weight = divisor;
				increased = 1;
			}
		}
	} while(increased);

	/* Scaling factor choosen, now scale */
	if(divisor > 1)
		for(i = 0; i < NUMVALS; ++i)
			self->node[i].weight /= divisor;
}
/*  */
/* heap() and adjust() maintain a list of binary trees as a
 * heap with the top indexing the binary tree on the list
 * which has the least weight or, in case of equal weights,
 * least depth in its longest path. The depth part is not
 * strictly necessary, but tends to avoid long codes which
 * might provoke rescaling.
 */

void heap(DQK_DSK_DRIVER *self, int list[], int length)
{
	int i;

	for(i = (length - 2) / 2; i >= 0; --i)
		adjust(self, list, i, length - 1);
}

/* Make a heap from a heap with a new top */

void adjust(DQK_DSK_DRIVER *self, int list[], int top, int bottom)
{
	int k, temp;

	k = 2 * top + 1;	/* left child of top */
	temp = list[top];	/* remember root node of top tree */
	if( k <= bottom) {
		if( k < bottom && cmptrees(self, list[k], list[k + 1]))
			++k;

		/* k indexes "smaller" child (in heap of trees) of top */
		/* now make top index "smaller" of old top and smallest child */
		if(cmptrees(self, temp, list[k])) {
			list[top] = list[k];
			list[k] = temp;
			/* Make the changed list a heap */
			adjust(self, list, k, bottom); /*recursive*/
		}
	}
}

/* Compare two trees, if a > b return true, else return false
 * note comparison rules in previous comments.
 */

char cmptrees(DQK_DSK_DRIVER *self, int a, int b)
{
	if(self->node[a].weight > self->node[b].weight)
		return 1;
	if(self->node[a].weight == self->node[b].weight)
		if(self->node[a].tdepth > self->node[b].tdepth)
			return 1;
	return 0;
}

static char maxchar(char a, char b)
{
	return a > b ? a : b;
}
/*  */
/* HUFFMAN ALGORITHM: develops the single element trees
 * into a single binary tree by forming subtrees rooted in
 * interior nodes having weights equal to the sum of weights of all
 * their descendents and having depth counts indicating the
 * depth of their longest paths.
 *
 * When all trees have been formed into a single tree satisfying
 * the heap property (on weight, with depth as a tie breaker)
 * then the binary code assigned to a leaf (value to be encoded)
 * is then the series of left (0) and right (1)
 * paths leading from the root to the leaf.
 * Note that trees are removed from the heaped list by
 * moving the last element over the top element and
 * reheaping the shorter list.
 */

void bld_tree(DQK_DSK_DRIVER *self, int list[], int len)
{
	int freenode;		/* next free node in tree */
	int lch, rch;		/* temporaries for left, right children */
	struct nd *frnp;	/* free node pointer */

	/* Initialize index to next available (non-leaf) node.
	 * Lower numbered nodes correspond to leaves (data values).
	 */
	freenode = NUMVALS;

	while(len > 1) {
		/* Take from list two btrees with least weight
		 * and build an interior node pointing to them.
		 * This forms a new tree.
		 */
		lch = list[0];	/* This one will be left child */

		/* delete top (least) tree from the list of trees */
		list[0] = list[--len];
		adjust(self, list, 0, len - 1);

		/* Take new top (least) tree. Reuse list slot later */
		rch = list[0];	/* This one will be right child */

		/* Form new tree from the two least trees using
		 * a free node as root. Put the new tree in the list.
		 */
		frnp = &self->node[freenode];	/* address of next free node */
		list[0] = freenode++;	/* put at top for now */
		frnp->lchild = lch;
		frnp->rchild = rch;
		frnp->weight = self->node[lch].weight + self->node[rch].weight;
		frnp->tdepth = 1 + maxchar(self->node[lch].tdepth, self->node[rch].tdepth);
		/* reheap list  to get least tree at top*/
		adjust(self, list, 0, len - 1);
	}
	self->dctreehd = list[0];	/*head of final tree */
}
/* Initialize all nodes to single element binary trees
 * with zero weight and depth.
 */

void init_tree(DQK_DSK_DRIVER *self)
{
	int i;

	for(i = 0; i < NUMNODES; ++i) {
		self->node[i].weight = 0;
		self->node[i].tdepth = 0;
		self->node[i].lchild = NOCHILD;
		self->node[i].rchild = NOCHILD;
	}
}

void init_enc(DQK_DSK_DRIVER *self)
{
	int i;

	/* Initialize encoding table */
	for(i = 0; i < NUMVALS; ++i) {
		self->codelen[i] = 0;
	}
}
/*  */
/* Recursive routine to walk the indicated subtree and level
 * and maintain the current path code in bstree. When a leaf
 * is found the entire code string and length are put into
 * the encoding table entry for the leaf's data value.
 *
 * Returns ERROR if codes are too long.
 */

int buildenc(DQK_DSK_DRIVER *self,
int level, /* level of tree being examined, from zero */
int root) /* root of subtree is also data value if leaf */
{
	int l, r;

	l = self->node[root].lchild;
	r = self->node[root].rchild;

	if( l == NOCHILD && r == NOCHILD) {
		/* Leaf. Previous path determines bit string
		 * code of length level (bits 0 to level - 1).
		 * Ensures unused code bits are zero.
		 */
		self->codelen[root] = level;
		self->code[root] = self->tcode & ((unsigned int)(~0) >> ((sizeof(int) * 8) - level));
		return (level > 16) ? ERROR : 0;
	} else {
		if( l != NOCHILD) {
			/* Clear path bit and continue deeper */
			self->tcode &= ~(1 << level);
			/* NOTE RECURSION */
			if(buildenc(self, level + 1, l) == ERROR)
				return ERROR;
		}
		if(r != NOCHILD) {
			/* Set path bit and continue deeper */
			self->tcode |= 1 << level;
			/* NOTE RECURSION */
			if(buildenc(self, level + 1, r) == ERROR)
				return ERROR;
		}
	}
	return 0;	/* if we got here we're ok so far */
}
/*  */
/* Write out the header of the compressed file */

dsk_err_t wrt_head(DQK_DSK_DRIVER *self, FILE *ob)
{
	int i, k, l, r;
	int numnodes;		/* nbr of nodes in simplified tree */
	dsk_err_t err;

	err = putwe(self, RECOGNIZE, ob);	/* identifies as compressed */
	if (err) return err;
	err = putwe(self, self->crc, ob);		/* unsigned sum of original data */
	if (err) return err;

	/* Record the original file name. */

	fwrite(self->dqk_truename, 1, 1 + strlen(self->dqk_truename), ob);	

	/* Write out a simplified decoding tree. Only the interior
	 * nodes are written. When a child is a leaf index
	 * (representing a data value) it is recoded as
	 * -(index + 1) to distinguish it from interior indexes
	 * which are recoded as positive indexes in the new tree.
	 * Note that this tree will be empty for an empty file.
	 */

	numnodes = self->dctreehd < NUMVALS ? 0 : self->dctreehd - (NUMVALS -1);
	err = putwe(self, numnodes, ob);
	if (err) return err;

	for(k = 0, i = self->dctreehd; k < numnodes; ++k, --i) {
		l = self->node[i].lchild;
		r = self->node[i].rchild;
		l = l < NUMVALS ? -(l + 1) : self->dctreehd - l;
		r = r < NUMVALS ? -(r + 1) : self->dctreehd - r;
		err = putwe(self, l, ob);	/* left child */
		if (err) return err;
		err = putwe(self, r, ob);	/* right child */
		if (err) return err;
	}
	return DSK_ERR_OK;
}
/*  */
/* Get an encoded byte or EOF. Reads from specified stream AS NEEDED.
 *
 * There are two unsynchronized bit-byte relationships here.
 * The input stream bytes are converted to bit strings of
 * various lengths via the static variables named c...
 * These bit strings are concatenated without padding to
 * become the stream of encoded result bytes, which this
 * function returns one at a time. The EOF (end of file) is
 * converted to SPEOF for convenience and encoded like any
 * other input value. True EOF is returned after that.
 *
 * The original gethuff() called a seperate function,
 * getbit(), but that more readable version was too slow.
 */

int gethuff(DQK_DSK_DRIVER *self)	/*  Returns byte values except for EOF */
{
	unsigned int rbyte;	/* Result byte value */
	char need;		/* numbers of bits */

	rbyte = 0;
	need = 8;		/* build one byte per call */

	/* Loop to build a byte of encoded data
	 * Initialization forces read the first time
	 */

loop:
	if(self->cbitsrem >= need) {
		/* Current code fullfills our needs */
		if(need == 0)
			return rbyte & 0xff;
		/* Take what we need */
 		rbyte |= self->ccode << (8 - need);
		/* And leave the rest */
		self->ccode >>= need;
		self->cbitsrem -= need;
		return rbyte & 0xff;
	}

	/* We need more than current code */
	if(self->cbitsrem > 0) {
		/* Take what there is */
		rbyte |= self->ccode << (8 - need);
		need -= self->cbitsrem;
	}
	/* No more bits in current code string */
	if(self->curin == SPEOF) {
		/* The end of file token has been encoded. If
		 * result byte has data return it and do EOF next time
		 */
		self->cbitsrem = 0;

		return (need == 8) ? EOF : rbyte & 0xff;
	}

	/* Get an input byte */
	if((self->curin = getcnr(self)) == EOF)
		self->curin = SPEOF;	/* convenient for encoding */

	/* Get the new byte's code */
	self->ccode = self->code[self->curin];
	self->cbitsrem = self->codelen[self->curin];

	goto loop;
}


dsk_err_t putwe(DQK_DSK_DRIVER *self, int w, FILE *iob)
{
	if (fputc(w & 0xFF, iob) == EOF) return DSK_ERR_SYSERR;
	if (fputc(w >> 8  , iob) == EOF) return DSK_ERR_SYSERR;
	return DSK_ERR_OK;
}


