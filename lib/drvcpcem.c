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

/* Access functions for CPCEMU discs */

#include "drvi.h"
#include "drvcpcem.h"


/* This struct contains function pointers to the driver's functions, and the
 * size of its DSK_DRIVER subclass */

DRV_CLASS dc_cpcemu = 
{
	sizeof(CPCEMU_DSK_DRIVER),
	"dsk",
	"CPCEMU .DSK driver",
	cpcemu_open,	/* open */
	cpcemu_creat,	/* create new */
	cpcemu_close,	/* close */
	cpcemu_read,	/* read sector, working from physical address */
	cpcemu_write,	/* write sector, working from physical address */
	cpcemu_format,	/* format track, physical */
	NULL,		/* get geometry */
	cpcemu_secid,	/* logical sector ID */
	cpcemu_xseek,	/* seek to track */
	cpcemu_status,	/* get drive status */
	cpcemu_xread,	/* read sector */
	cpcemu_xwrite,	/* write sector */ 
};

/* Open DSK image, checking for the magic number */
dsk_err_t cpcemu_open(DSK_DRIVER *self, const char *filename)
{
	CPCEMU_DSK_DRIVER *cpc_self;
	
	/* Sanity check: Is this meant for our driver? */
	if (self->dr_class != &dc_cpcemu) return DSK_ERR_BADPTR;
	cpc_self = (CPCEMU_DSK_DRIVER *)self;

	cpc_self->cpc_fp = fopen(filename, "r+b");
	if (!cpc_self->cpc_fp) 
	{
		cpc_self->cpc_readonly = 1;
		cpc_self->cpc_fp = fopen(filename, "rb");
	}
	if (!cpc_self->cpc_fp) return DSK_ERR_NOTME;
	/* Check for CPCEMU signature */
	if (fread(cpc_self->cpc_dskhead, 1, 256, cpc_self->cpc_fp) < 256) 
		return DSK_ERR_NOTME;

        if (memcmp("MV - CPC", cpc_self->cpc_dskhead, 8) &&
            memcmp("EXTENDED", cpc_self->cpc_dskhead, 8)) return DSK_ERR_NOTME; 
	/* OK, got signature. */
	cpc_self->cpc_trkhead[0] = 0;
	return DSK_ERR_OK;
}

/* Create DSK image */
dsk_err_t cpcemu_creat(DSK_DRIVER *self, const char *filename)
{
	CPCEMU_DSK_DRIVER *cpc_self;
	
	/* Sanity check: Is this meant for our driver? */
	if (self->dr_class != &dc_cpcemu) return DSK_ERR_BADPTR;
	cpc_self = (CPCEMU_DSK_DRIVER *)self;

	cpc_self->cpc_fp = fopen(filename, "w+b");
	cpc_self->cpc_readonly = 0;
	if (!cpc_self->cpc_fp) return DSK_ERR_SYSERR;
        memset(cpc_self->cpc_dskhead, 0, 256);
        strcpy((char *)cpc_self->cpc_dskhead,
		"MV - CPCEMU Disk-File\r\nDisk-Info\r\n(LIBDSK)");
        if (fwrite(cpc_self->cpc_dskhead, 1 , 256, cpc_self->cpc_fp) < 256) return DSK_ERR_SYSERR;
	cpc_self->cpc_trkhead[0] = 0;
	return DSK_ERR_OK;
}


dsk_err_t cpcemu_close(DSK_DRIVER *self)
{
	CPCEMU_DSK_DRIVER *cpc_self;

	if (self->dr_class != &dc_cpcemu) return DSK_ERR_BADPTR;
	cpc_self = (CPCEMU_DSK_DRIVER *)self;

	if (cpc_self->cpc_fp) 
	{
		if (fclose(cpc_self->cpc_fp) == EOF) return DSK_ERR_SYSERR;
		cpc_self->cpc_fp = NULL;
	}
	return DSK_ERR_OK;	
}






/* Find the offset in a DSK for a particular cylinder/head. 
 *
 * CPCEMU DSK files work in "tracks". For a single-sided disk, track number
 * is the same as cylinder number. For a double-sided disk, track number is
 * (2 * cylinder + head). This is independent of disc format.
 */
static long lookup_track(CPCEMU_DSK_DRIVER *self, const DSK_GEOMETRY *geom,
			  dsk_pcyl_t cylinder, dsk_phead_t head)
{
	unsigned char *b;
	dsk_ltrack_t track;
	long trk_offset;
	int nt;

	if (!self->cpc_fp) return -1;

	/* [LIBDSK v0.6.0] Compare with our header, not the passed
	 * geometry */
	/* Seek off the edge of the drive? Note that we allow one 
	 * extra cylinder & one extra head, so that we can move to 
	 * a blank track to format it. */
	if (cylinder >  self->cpc_dskhead[0x30]) return -1;
	if (head     >  self->cpc_dskhead[0x31]) return -1;

	/* Convert cylinder & head to CPCEMU "track" */

	track = cylinder;
	if (self->cpc_dskhead[0x31] > 1) track *= 2;
	track += head;

        /* Look up the cylinder and head using the header. This behaves 
         * differently in normal and extended DSK files */
	
	if (!memcmp(self->cpc_dskhead, "EXTENDED", 8))
	{
		trk_offset = 256;	/* DSK header = 256 bytes */
		b = self->cpc_dskhead + 0x34;
		for (nt = 0; nt < track; nt++)
		{
			trk_offset += 256 * (1 + b[nt]);
		}
	}
	else	/* Normal; all tracks have the same length */
	{
		trk_offset = (self->cpc_dskhead[0x33] * 256);
		trk_offset += self->cpc_dskhead[0x32];

		trk_offset *= track;		/* No. of tracks */
		trk_offset += 256;		/* DSK header */	
	}
	return trk_offset;
}





/* Seek to a cylinder. Checks if that particular cylinder exists. 
 * We test for the existence of a cylinder by looking for Track <n>, Head 0.
 * Fortunately the DSK format does not allow for discs with different numbers
 * of tracks on each side (though this is obviously possible with a real disc)
 * so if head 0 exists then the whole cylinder does. 


static dsk_err_t seek_cylinder(CPCEMU_DSK_DRIVER *self, DSK_GEOMETRY *geom, int cylinder)
{
	long nr;
	if (!self->cpc_fp) return DSK_ERR_NOTRDY;

	// Check if the DSK image goes out to the correct cylinder 
	nr = lookup_track(self, geom, cylinder, 0);
	
	if (nr < 0) return DSK_ERR_SEEKFAIL;
	return DSK_ERR_OK;
}
*/

/* Load the "Track-Info" header for the given cylinder and head */
static dsk_err_t load_track_header(CPCEMU_DSK_DRIVER *self, const DSK_GEOMETRY *geom, 
					int cylinder, int head)
{
        long track;

	track = lookup_track(self, geom, cylinder, head);
        if (track < 0) return DSK_ERR_SEEKFAIL;       /* Bad track */
        fseek(self->cpc_fp, track, SEEK_SET);
        if (fread(self->cpc_trkhead, 1, 256, self->cpc_fp) < 256)
                return DSK_ERR_NOADDR;              /* Missing address mark */
        if (memcmp(self->cpc_trkhead, "Track-Info", 10))
        {
                return DSK_ERR_NOADDR;
        }
	return DSK_ERR_OK;
}


/* Read a sector ID from a given track */
dsk_err_t cpcemu_secid(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
                                dsk_pcyl_t cyl, dsk_phead_t head, DSK_FORMAT *result)
{
	CPCEMU_DSK_DRIVER *cpc_self;
	dsk_err_t e;
	int offs;

	if (!self || !geom || !result || self->dr_class != &dc_cpcemu) 
		return DSK_ERR_BADPTR;
	cpc_self = (CPCEMU_DSK_DRIVER *)self;
	
	if (!cpc_self->cpc_fp) return DSK_ERR_NOTRDY;

	e = load_track_header(cpc_self, geom, cyl, head);
	if (e) return e;

	/* Offset of the chosen sector header */
	++cpc_self->cpc_sector;
	offs = 0x18 + 8 * (cpc_self->cpc_sector % cpc_self->cpc_trkhead[0x15]);

	result->fmt_cylinder = cpc_self->cpc_trkhead[offs];
	result->fmt_head     = cpc_self->cpc_trkhead[offs+1];
	result->fmt_sector   = cpc_self->cpc_trkhead[offs+2];
	result->fmt_secsize  = 128 << cpc_self->cpc_trkhead[offs+3];
	return DSK_ERR_OK;	
}


/* Find the offset of a sector in the current track 
 * Enter with cpc_trkhead loaded and the file pointer 
 * just after it (ie, you have just called load_track_header() ) */

static long sector_offset(CPCEMU_DSK_DRIVER *self, dsk_psect_t sector, size_t *seclen,
			      unsigned char **secid)
{
	int maxsec = self->cpc_trkhead[0x15];
	long offset = 0;
	int n;

	/* Pointer to sector details */
	*secid = self->cpc_trkhead + 0x18;

	/* Length of sector */	
	*seclen = (0x80 << self->cpc_trkhead[0x14]);

	/* Extended DSKs have individual sector sizes */
	if (!memcmp(self->cpc_dskhead, "EXTENDED", 8))
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
static dsk_err_t seekto_sector(CPCEMU_DSK_DRIVER *self, const DSK_GEOMETRY *geom,
		int cylinder, int head, int cyl_expected,
		int head_expected, int sector, size_t *len)
{
        int offs;
        size_t seclen;
	dsk_err_t err;
	unsigned char *secid;
	long trkbase;

        err = load_track_header(self, geom, cylinder, head);
        if (err) return err;
	trkbase = ftell(self->cpc_fp);
	offs = sector_offset(self, sector, &seclen, &secid);
	if (offs < 0) return DSK_ERR_NOADDR;	/* Sector not found */

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

	fseek(self->cpc_fp, trkbase + offs, SEEK_SET);

	return err;
}


/* Read a sector */
dsk_err_t cpcemu_read(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
                             void *buf, dsk_pcyl_t cylinder,
                              dsk_phead_t head, dsk_psect_t sector)
{
	return cpcemu_xread(self, geom, buf, cylinder, head, cylinder,
			    head, sector, geom->dg_secsize);
}

dsk_err_t cpcemu_xread(DSK_DRIVER *self, const DSK_GEOMETRY *geom, void *buf, 
			      dsk_pcyl_t cylinder,   dsk_phead_t head, 
			      dsk_pcyl_t cyl_expect, dsk_phead_t head_expect,
			      dsk_psect_t sector, size_t sector_size)
{
	CPCEMU_DSK_DRIVER *cpc_self;
	dsk_err_t err;
	size_t len = geom->dg_secsize;

	if (!buf || !geom || !self) return DSK_ERR_BADPTR;
	if (self->dr_class != &dc_cpcemu) return DSK_ERR_BADPTR;
	cpc_self = (CPCEMU_DSK_DRIVER *)self;

	err  = seekto_sector(cpc_self, geom, cylinder,head, 
					cyl_expect, head_expect, sector, &len);
	if (err == DSK_ERR_DATAERR || err == DSK_ERR_OK)
	{
		if (len > sector_size) len = sector_size;
		if (fread(buf, 1, len, cpc_self->cpc_fp) < len) 
			err = DSK_ERR_DATAERR;
	}
	return err;
}		


/* Write a sector */
dsk_err_t cpcemu_write(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
                             const void *buf, dsk_pcyl_t cylinder,
                              dsk_phead_t head, dsk_psect_t sector)
{
	return cpcemu_xwrite(self, geom, buf, cylinder, head, cylinder,head,
				sector, geom->dg_secsize);
}

dsk_err_t cpcemu_xwrite(DSK_DRIVER *self, const DSK_GEOMETRY *geom, 
			      const void *buf, 
			      dsk_pcyl_t cylinder,   dsk_phead_t head, 
			      dsk_pcyl_t cyl_expect, dsk_phead_t head_expect,
			      dsk_psect_t sector, size_t sector_size)
{
	CPCEMU_DSK_DRIVER *cpc_self;
	dsk_err_t err;
	size_t len = geom->dg_secsize;

	if (!buf || !geom || !self) return DSK_ERR_BADPTR;
	if (self->dr_class != &dc_cpcemu) return DSK_ERR_BADPTR;
	cpc_self = (CPCEMU_DSK_DRIVER *)self;

	if (cpc_self->cpc_readonly) return DSK_ERR_RDONLY;

	err  = seekto_sector(cpc_self, geom, cylinder,head,  
				cyl_expect, head_expect, sector, &len);
	if (err == DSK_ERR_DATAERR || err == DSK_ERR_OK)
	{
		if (len > sector_size) len = sector_size;
		if (fwrite(buf, 1, len, cpc_self->cpc_fp) < len)
			err = DSK_ERR_DATAERR;
        }
	return err;
}


/* Format a track on a DSK. Can grow the DSK file. */
dsk_err_t cpcemu_format(DSK_DRIVER *self, DSK_GEOMETRY *geom,
                                dsk_pcyl_t cylinder, dsk_phead_t head,
                                const DSK_FORMAT *format, unsigned char filler)
{
	CPCEMU_DSK_DRIVER *cpc_self;
	int n, img_trklen, trklen, trkoff, trkno, ext, seclen;
	unsigned char oldhead[256];

	if (!format || !geom || !self) return DSK_ERR_BADPTR;
	if (self->dr_class != &dc_cpcemu) return DSK_ERR_BADPTR;
	cpc_self = (CPCEMU_DSK_DRIVER *)self;

	if (!cpc_self->cpc_fp)      return DSK_ERR_NOTRDY;
	if (cpc_self->cpc_readonly) return DSK_ERR_RDONLY;

	ext = 0;
	memcpy(oldhead, cpc_self->cpc_dskhead, 256);

/* 1. Only if the DSK has either (1 track & 1 head) or (2 heads) can we
 *   format the second head
 */
	if (head)
	{
		if (cpc_self->cpc_dskhead[0x31] == 1 && 
		    cpc_self->cpc_dskhead[0x30] > 1) return DSK_ERR_RDONLY;

		if (cpc_self->cpc_dskhead[0x31] == 1) 
			cpc_self->cpc_dskhead[0x31] = 2;
	}
/* 2. Find out the CPCEMU number of the new cylinder/head */

	if (cpc_self->cpc_dskhead[0x31] < 1) cpc_self->cpc_dskhead[0x31] = 1;
	trkno = cylinder;
	trkno *= cpc_self->cpc_dskhead[0x31];
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
/* 4. Work out if this length is suitable
 */
	if (!memcmp(cpc_self->cpc_dskhead, "EXTENDED", 8))
	{
		unsigned char *b;
		/* For an extended DSK, work as follows: 
		 * If the track is reformatting an existing one, 
		 * the length must be <= what's there. 
		 * If the track is new, it must be contiguous with the 
		 * others */

		ext = 1;
		img_trklen = (cpc_self->cpc_dskhead[0x34 + trkno] * 256) + 256;
		if (img_trklen)
		{
			if (trklen > img_trklen) return DSK_ERR_RDONLY;
		}
		else if (trkno > 0) 
		{
			if (!cpc_self->cpc_dskhead[0x34 + trkno - 1]) 
			{
				memcpy(cpc_self->cpc_dskhead, oldhead, 256);
				return DSK_ERR_RDONLY;
			}
		}
		/* Work out where the track should be. */
                b = cpc_self->cpc_dskhead + 0x34;
       		trkoff = 256; 
	        for (n = 0; n < trkno; n++)
                {
                        trkoff += 256 * (1 + b[n]);
                }
		/* Store the length of the new track */
		if (!b[n]) b[n] = (unsigned char)((trklen >> 8) - 1);
	}
	else
	{
		img_trklen = cpc_self->cpc_dskhead[0x32] + 256 * 
			     cpc_self->cpc_dskhead[0x33];
		/* If no tracks formatted, or just the one track, length can
                 * be what we like */
		if ( (cpc_self->cpc_dskhead[0x30] == 0) ||
		     (cpc_self->cpc_dskhead[0x30] == 1 && 
                      cpc_self->cpc_dskhead[0x31] == 1) )
		{
			if (trklen > img_trklen)
			{
				cpc_self->cpc_dskhead[0x32] = (unsigned char)(trklen & 0xFF);
				cpc_self->cpc_dskhead[0x33] = (unsigned char)(trklen >> 8);
				img_trklen = trklen;	
			}
		}
		if (trklen > img_trklen)
		{
			memcpy(cpc_self->cpc_dskhead, oldhead, 256);
			return DSK_ERR_RDONLY;
		}
		trkoff = 256 + (trkno * img_trklen);
	}
/* Seek to the track. */
	fseek(cpc_self->cpc_fp, trkoff, SEEK_SET);
	/* Now generate and write a Track-Info buffer */
	memset(cpc_self->cpc_trkhead, 0, sizeof(cpc_self->cpc_trkhead));

	strcpy((char *)cpc_self->cpc_trkhead, "Track-Info\r\n");

	cpc_self->cpc_trkhead[0x10] = (unsigned char)cylinder;
	cpc_self->cpc_trkhead[0x11] = (unsigned char)head;
	cpc_self->cpc_trkhead[0x14] = dsk_get_psh(format[0].fmt_secsize);
	cpc_self->cpc_trkhead[0x15] = (unsigned char)geom->dg_sectors;
	cpc_self->cpc_trkhead[0x16] = geom->dg_fmtgap;
	cpc_self->cpc_trkhead[0x17] = filler;
	for (n = 0; n < geom->dg_sectors; n++)
	{
		cpc_self->cpc_trkhead[0x18 + 8*n] = (unsigned char)format[n].fmt_cylinder;
		cpc_self->cpc_trkhead[0x19 + 8*n] = (unsigned char)format[n].fmt_head;
		cpc_self->cpc_trkhead[0x1A + 8*n] = (unsigned char)format[n].fmt_sector;
		cpc_self->cpc_trkhead[0x1B + 8*n] = dsk_get_psh(format[n].fmt_secsize);
		if (ext)
		{
			seclen = format[n].fmt_secsize;
			cpc_self->cpc_trkhead[0x1E + 8 * n] = (unsigned char)(seclen & 0xFF);
			cpc_self->cpc_trkhead[0x1F + 8 * n] = (unsigned char)(seclen >> 8);
		}
	}
	if (fwrite(cpc_self->cpc_trkhead, 1, 256, cpc_self->cpc_fp) < 256)
	{
		memcpy(cpc_self->cpc_dskhead, oldhead, 256);
		return DSK_ERR_RDONLY;
	}
	/* Track header written. Write sectors */
	for (n = 0; n < geom->dg_sectors; n++)
	{
		int m;
		seclen = format[n].fmt_secsize;
		for (m = 0; m < seclen; m++)
		{
			if (fputc(filler, cpc_self->cpc_fp) == EOF)
			{
				memcpy(cpc_self->cpc_dskhead, oldhead, 256);
				return DSK_ERR_RDONLY;
			}
		}
	}
	if (cylinder >= cpc_self->cpc_dskhead[0x30])
	{
		cpc_self->cpc_dskhead[0x30] = (unsigned char)(cylinder + 1);
	}
	/* Track formatted OK. Now write back the modified DSK header */
	fseek(cpc_self->cpc_fp, 0, SEEK_SET);
	if (fwrite(cpc_self->cpc_dskhead, 1, 256, cpc_self->cpc_fp) < 256)
	{
		memcpy(cpc_self->cpc_dskhead, oldhead, 256);
		return DSK_ERR_RDONLY;
	}
	/* If the disc image has grown because of this, record this in the
	 * disc geometry struct */
	
	if (geom->dg_heads     < cpc_self->cpc_dskhead[0x31]) 
		geom->dg_heads     = cpc_self->cpc_dskhead[0x31];
	if (geom->dg_cylinders < cpc_self->cpc_dskhead[0x30])
		geom->dg_cylinders = cpc_self->cpc_dskhead[0x30]; 
		
	return DSK_ERR_OK;
}



/* Seek to a cylinder. */
dsk_err_t cpcemu_xseek(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
                                dsk_pcyl_t cyl, dsk_phead_t head)
{
	CPCEMU_DSK_DRIVER *cpc_self;

	if (!self || !geom || self->dr_class != &dc_cpcemu) 
		return DSK_ERR_BADPTR;
	cpc_self = (CPCEMU_DSK_DRIVER *)self;
	
	if (!cpc_self->cpc_fp) return DSK_ERR_NOTRDY;

	// See if the cylinder & head are in range
	if (cyl  > cpc_self->cpc_dskhead[0x30] ||
            head > cpc_self->cpc_dskhead[0x31]) return DSK_ERR_SEEKFAIL;
	return DSK_ERR_OK;
}


dsk_err_t cpcemu_status(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
                      dsk_phead_t head, unsigned char *result)
{
        CPCEMU_DSK_DRIVER *cpc_self;

        if (!self || !geom || self->dr_class != &dc_cpcemu) return DSK_ERR_BADPTR;
        cpc_self = (CPCEMU_DSK_DRIVER *)self;

        if (!cpc_self->cpc_fp) *result &= ~DSK_ST3_READY;
        if (cpc_self->cpc_readonly) *result |= DSK_ST3_RO;
	return DSK_ERR_OK;
}

