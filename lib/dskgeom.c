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

/* Disc geometry probe and related code */

#include "drvi.h"

/* Probe the geometry of a disc. This will use the boot sector or the
 * driver's own probe */

dsk_err_t dsk_getgeom(DSK_DRIVER *self, DSK_GEOMETRY *geom)
{
        DRV_CLASS *dc; 
	DSK_FORMAT secid;
	dsk_err_t e;
	unsigned char *secbuf;
	unsigned long dsksize;

        if (!self || !geom || !self->dr_class) return DSK_ERR_BADPTR;

	/* Check if the driver has overridden this function. If it has,
	 * then use its geometry probe, which is probably more limited. */
	dc = self->dr_class; 
	memset(geom, 0, sizeof(*geom));

	if (dc->dc_getgeom)
	{
		e = (dc->dc_getgeom)(self, geom);
		if (e != DSK_ERR_NOTME) return e;	
	}	

	/* Switch to a minimal format */
	e = dg_stdformat(geom, FMT_180K, NULL, NULL);
	if (e) return e;
	/* Allocate buffer for boot sector (512 bytes) */
	secbuf = dsk_malloc(geom->dg_secsize);
	if (!secbuf) return DSK_ERR_NOMEM;

	/* Check for CPC6128 type discs. Also probe the data rate; if we get a 
	 * missing address mark, then the data rate is wrong.
	 */ 
	e = dsk_lsecid(self, geom, 0, &secid);
	if (e == DSK_ERR_NOADDR)
	{
		geom->dg_datarate = RATE_HD;
		e = dsk_lsecid(self, geom, 0, &secid);
	}
	if (!e)	/* We could get the sector ID */
	{
		if ((secid.fmt_sector & 0xC0) == 0x40) 	/* CPC system */
		{
			dsk_free(secbuf);
			return dg_stdformat(geom, FMT_CPCSYS, NULL, NULL);
		}
		if ((secid.fmt_sector & 0xC0) == 0xC0)	/* CPC data */
		{
			dsk_free(secbuf);
			return dg_stdformat(geom, FMT_CPCDATA, NULL, NULL);
		}
		/* [v0.6.0] Handle discs with non-512 byte sectors */
		if (secid.fmt_secsize == 256)
		{
			e = dg_stdformat(geom, FMT_ACORN160, NULL, NULL);
			if (!e) e = dsk_lread(self, geom, secbuf, 0);
			if (e)
			{
				dsk_free(secbuf);
				return DSK_ERR_BADFMT;
			}
			/* Acorn ADFS discs have a size in sectors at 0xFC in the
			 * first sector */
			dsksize = secbuf[0xFC] + 256 * secbuf[0xFD] +
				65536L * secbuf[0xFE];
			dsk_free(secbuf);
			if (dsksize ==  640) return dg_stdformat(geom, FMT_ACORN160, NULL, NULL);
			if (dsksize == 1280) return dg_stdformat(geom, FMT_ACORN320, NULL, NULL);
			if (dsksize == 2560) return dg_stdformat(geom, FMT_ACORN640, NULL, NULL);
			return DSK_ERR_BADFMT;
		}
		if (secid.fmt_secsize == 1024)
		{
			dsk_free(secbuf);
			/* Switch to a format with 1k sectors */
			if (geom->dg_datarate == RATE_HD)
				e = dg_stdformat(geom, FMT_ACORN1600, NULL, NULL);	
			else	e = dg_stdformat(geom, FMT_ACORN800, NULL, NULL);
			if (e) return e;
			/* Allocate buffer for boot sector (1k bytes) */
			secbuf = dsk_malloc(geom->dg_secsize);
			if (!secbuf) return DSK_ERR_NOMEM;
			e = dsk_lread(self, geom, secbuf, 0);
			if (!e)
			{
				
				dsksize = secbuf[0xFC] + 256 * secbuf[0xFD] +
					65536L * secbuf[0xFE];
				dsk_free(secbuf);
				/* Check for 1600k-format */
				if (geom->dg_datarate == RATE_HD)
				{
				/* XXX Need a better check for Acorn 1600k */
					return DSK_ERR_OK;
				}
				/* Check for D-format magic */
				if (dsksize == 3200) return DSK_ERR_OK;
				/* Check for E-format magic */
				if (secbuf[4] == 10 && secbuf[5] == 5 &&
				    secbuf[6] == 2  && secbuf[7] == 2)
						return DSK_ERR_OK;
			}
			else	dsk_free(secbuf);
			return DSK_ERR_BADFMT;
		}	
		/* Can't handle other discs with non-512 sector sizes. */
		if ((secid.fmt_secsize != 512))
		{
			dsk_free(secbuf);
			return DSK_ERR_BADFMT;
		}
	}
	/* If the driver couldn't do a READ ID call, then ignore it */
	if (e == DSK_ERR_NOTIMPL) e = DSK_ERR_OK;
	/* Try to ID the disc from its boot sector */
	if (!e) e = dsk_lread(self, geom, secbuf, 0);
	if (e) 
	{ 	
		dsk_free(secbuf);
		return e; 
	}
	/* We have the sector. Let's try to guess what it is */
	e = dg_dosgeom(geom, secbuf);	
	if (e == DSK_ERR_BADFMT) e = dg_pcwgeom(geom, secbuf);
	if (e == DSK_ERR_BADFMT) e = dg_cpm86geom(geom, secbuf);
	
	dsk_free(secbuf);
	return e;
}


/* Interpret a DOS superblock */
dsk_err_t dg_dosgeom(DSK_GEOMETRY *self, const unsigned char *bootsect)
{
	dsk_lsect_t lsmax;

	if (!self || !bootsect) return DSK_ERR_BADPTR;

/* If the boot sector starts 0xE9 or 0xEB, it's DOS. If it starts with
 * three zeroes, it's Atari. 
 *  In particular, we have to be careful not to try to identify a 
 * PCW 180k floppy, which starts 0x00 0x00 0x28 0x09 */

	if (bootsect[0] != 0xE9 && bootsect[0] != 0xEB)
	{
		if (bootsect[0] || bootsect[1] || bootsect[2]) 
			return DSK_ERR_BADFMT;
	}

	/* Reject fake DOS bootsectors created by 144FEAT */ 	
	if (bootsect[511] == 144 || bootsect[511] == 72 || bootsect[511] == 12)
		return DSK_ERR_BADFMT;

	self->dg_secsize   = bootsect[11] + 256 * bootsect[12];
	if (!self->dg_secsize) self->dg_secsize = 512;
	self->dg_secbase   = 1;
	self->dg_heads     = bootsect[26] + 256 * bootsect[27];
	self->dg_sectors   = bootsect[24] + 256 * bootsect[25];
	if (!self->dg_heads || !self->dg_sectors) return DSK_ERR_BADFMT;
	lsmax = bootsect[19] + 256 * bootsect[20];
	lsmax /= self->dg_heads;
	lsmax /= self->dg_sectors;
	self->dg_cylinders = lsmax; 
	/* DOS boot sector doesn't store the data rate. We guess that if there are >12
	 * sectors per track, it must have used high density to get them all in */
	self->dg_datarate  = (self->dg_sectors >= 12) ? RATE_HD : RATE_SD;
	/* Similarly it doesn't store the gap lengths: */
	switch(self->dg_sectors)
	{
		case 8:  self->dg_rwgap = 0x2A; self->dg_fmtgap = 0x50; break;
		case 9:  self->dg_rwgap = 0x2A; self->dg_fmtgap = 0x52; break;
		case 10: self->dg_rwgap = 0x0C; self->dg_fmtgap = 0x17; break;
		case 15: self->dg_rwgap = 0x1B; self->dg_fmtgap = 0x50; break;
		case 18: self->dg_rwgap = 0x1B; self->dg_fmtgap = 0x50; break;
		default: self->dg_rwgap = 0x2A; self->dg_fmtgap = 0x52; break;
	}
	self->dg_fm      = 0;
	self->dg_nomulti = 0;

	return DSK_ERR_OK;
}


/* Interpret a PCW superblock */
dsk_err_t dg_pcwgeom(DSK_GEOMETRY *dg, const unsigned char *bootsec)
{
	static unsigned char defsec[10] = { 0, 0, 40, 9, 2, 1, 3, 2, 42, 82 };
	static unsigned char alle5[10]  = { 0xE5, 0xE5, 0xE5, 0xE5, 0xE5,
					    0xE5, 0xE5, 0xE5, 0xE5, 0xE5 };

	/* Treat all 0xE5s as 180k */
	if (!memcmp(bootsec, alle5, 10)) bootsec = defsec;
	/* Check for PCW16 boot/root format */
	if (bootsec[0] == 0xE9 || bootsec[0] == 0xEA)
	{
		if (memcmp(bootsec + 0x2B, "CP/M", 4) ||
		    memcmp(bootsec + 0x33, "DSK", 3)  ||
		    memcmp(bootsec + 0x7C, "CP/M", 4)) return DSK_ERR_BADFMT;
		/* Detected PCW16 boot+root, disc spec at 80h */
		bootsec += 0x80;
	}
	if (bootsec[0] != 3 && bootsec[0] != 0) return DSK_ERR_BADFMT;

	switch(bootsec[1] & 3)
	{
		case 0: dg->dg_heads = 1; dg->dg_sidedness = SIDES_ALT; break;
		case 1: dg->dg_heads = 2; dg->dg_sidedness = SIDES_ALT; break;
		case 2: dg->dg_heads = 2; dg->dg_sidedness = SIDES_OUTBACK; break;
		default: return DSK_ERR_BADFMT;
	}
	dg->dg_cylinders = bootsec[2];
	dg->dg_sectors   = bootsec[3];
	dg->dg_secbase   = 1;
	dg->dg_secsize   = 128;
	/* My PCW16 extension to the PCW superblock encodes data rate. Fancy that. */
	dg->dg_datarate  = (bootsec[1] & 0x40) ? RATE_HD : RATE_SD;
	dg->dg_fm      = 0;
	dg->dg_nomulti = 0;
	dg->dg_rwgap   = bootsec[8];
	dg->dg_fmtgap  = bootsec[9];
   dg->dg_secsize = 128 << bootsec[4];
   return DSK_ERR_OK;
}

/* Interpret a CP/M86 (floppy) superblock */
dsk_err_t dg_cpm86geom(DSK_GEOMETRY *dg, const unsigned char *bootsec)
{
	switch(bootsec[511])
	{
		case 0x00: return dg_stdformat(dg, FMT_160K, NULL, NULL);
		case 0x01: return dg_stdformat(dg, FMT_320K, NULL, NULL);
		case 0x0C: return dg_stdformat(dg, FMT_1200F, NULL, NULL);
		case 0x40:
		case 0x10: return dg_stdformat(dg, FMT_360K, NULL, NULL);
		case 0x11: return dg_stdformat(dg, FMT_720K, NULL, NULL);
		case 0x48: return dg_stdformat(dg, FMT_720F, NULL, NULL);
		case 0x90: return dg_stdformat(dg, FMT_1440F, NULL, NULL);
	}
	return DSK_ERR_BADFMT;
}

