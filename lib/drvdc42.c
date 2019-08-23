/***************************************************************************
 *                                                                         *
 *    LIBDSK: General floppy and diskimage access library                  *
 *    Copyright (C) 2001-2019  John Elliott <seasip.webmaster@gmail.com>   *
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

/* Support for the Apple Disk Copy 4.2 format, which seems to be the 
 * standard way to encapsulate a Macintosh GCR disk image */

#include <stdio.h>
#include "libdsk.h"
#include "drvi.h"
#include "drvldbs.h"
#include "drvdc42.h"

/* Since DC42 originates on the Mac, it may well have a resource fork 
 * and/or be encoded in MacBinary. */

/* There are four known formats: GCR 400k, GCR 800k, MFM 720k and MFM 1440k */
static LDBS_STATS known_formats[] = 
{
	{ 0, 79, 0, 0, 0,12, 8, 512, 512,12, 1 }, /* 400k */
	{ 0, 79, 0, 1, 0,12, 8, 512, 512,12, 1 }, /* 800k */
	{ 0, 79, 0, 1, 0, 9, 9, 512, 512, 9, 1 }, /* 720k */
	{ 0, 79, 0, 1, 0,18,18, 512, 512,18, 1 }, /* 1440k */
};

/* Signature for an LDBS block containing MacBinary data */
#define DC42_MBIN_BLOCK "MBIN"
/* Signature for an LDBS block containing DC42 resource fork */
#define DC42_RSRC_BLOCK "RSRC"


/* This struct contains function pointers to the driver's functions, and the
 * size of its DSK_DRIVER subclass */

DRV_CLASS dc_dc42 = 
{
	sizeof(DC42_DSK_DRIVER),
	&dc_ldbsdisk,	/* superclass */
	"dc42\0DC42\0",
	"Disk Copy 4.2",
	dc42_open,	/* open */
	dc42_creat,	/* create new */
	dc42_close,	/* close */
};

unsigned long be_peek4(unsigned char *src)
{
        unsigned long u = src[0];
        u = (u << 8) | src[1];
        u = (u << 8) | src[2];
        u = (u << 8) | src[3];
        return u;
}


void be_poke4(unsigned char *dest, unsigned long val)
{
	dest[0] = (val >> 24) & 0xFF;
	dest[1] = (val >> 16) & 0xFF;
	dest[2] = (val >> 8) & 0xFF;
	dest[3] = val & 0xFF;
}


dsk_err_t dc42_open(DSK_DRIVER *self, const char *filename)
{
	dsk_pcyl_t cylinder;
	dsk_phead_t head;
	dsk_psect_t sector;
	size_t secsize, trail, bufsize;
	int is_macbin = 0;
	unsigned char header[84];
	unsigned char macbin[128];	/* MacBinary header */	
	char comment[64], *comment_utf8;
	unsigned char *secbuf;
	int l, c;
	DC42_DSK_DRIVER *dcself;
	DSK_GEOMETRY geom;
	FILE *fp;
	unsigned long datalen, taglen;
	dsk_err_t err;
	long datapos, tagpos;

	/* Sanity check: Is this meant for our driver? */
	if (self->dr_class != &dc_dc42) return DSK_ERR_BADPTR;
	dcself = (DC42_DSK_DRIVER *)self;

	/* Save the filename, we'll want it when doing output */
	dcself->dc42_filename = dsk_malloc_string(filename);
	if (!dcself->dc42_filename) return DSK_ERR_NOMEM;

	fp = fopen(filename, "r+b");
	if (!fp)
	{
		dcself->dc42_super.ld_readonly = 1;
		fp = fopen(filename, "rb");
	}
	if (!fp) 
	{
		dsk_free(dcself->dc42_filename);
		return DSK_ERR_NOTME;
	}
	/* File is open. See if we can detect a MacBinary header */
	if (fread(macbin, 1, 128, fp) == 128)
	{
	/* TODO: This does not handle extended MacBinary II headers. 
	 * MacBinary parsing should probably also be moved into a separate
	 * file, since in the future LibDsk may have to access other types
	 * of MacBinary file */
		if (macbin[0] == 0  &&
		    macbin[1] >= 1  &&
		    macbin[1] <= 63 &&
		    !memcmp(macbin + 65, "dImg", 4))
		{
			/* It's MacBinary. */
			is_macbin = 1;
		}
	}
	/* Seek to start of data fork */
	if (fseek(fp, is_macbin ? 128 : 0, SEEK_SET) ||
	    fread(header, 1, sizeof(header), fp) < sizeof(header) ||
/* Check we can handle this disk. If header[0x50] > 3, it's an unknown 
 * encoding. Header[0x52-0x53] are a magic number. */
	    header[0x50] > 3 || 
	    header[0x52] != 1 || 
	    header[0x53] != 0)
	{
		fclose(fp);
		dsk_free(dcself->dc42_filename);
		return DSK_ERR_NOTME;
	}
	datalen = be_peek4(header + 0x40);	
	taglen  = be_peek4(header + 0x44);

	/* OK, we're ready to load the file. */
	err = ldbs_new(&dcself->dc42_super.ld_store, NULL, LDBS_DSK_TYPE);
	if (err)
	{
		dsk_free(dcself->dc42_filename);
		fclose(fp);
		return err;
	}
	/* Convert the comment from Mac-Roman to UTF8 and add it to
	 * the blockstore. */
	memcpy(comment, header + 1, 63);
	comment[header[0] & 0x3F] = 0;
	l = macroman_to_utf8(comment, NULL, -1);
	comment_utf8 = dsk_malloc(l + 2);
	if (!comment_utf8)
	{
		dsk_free(dcself->dc42_filename);
		ldbs_close(&dcself->dc42_super.ld_store);
		fclose(fp);
		return DSK_ERR_NOMEM;
	}
	macroman_to_utf8(comment, comment_utf8, -1);
	err = ldbs_put_comment(dcself->dc42_super.ld_store, comment_utf8);
	dsk_free(comment_utf8);

	/* Byte 0x51 of the header gives sidedness, plus GCR subtype 
	 * (Mac / Lisa / Apple IIgs) for GCR disks, or sector size for MFM 
	 * disks. */
	if (header[0x50] < 2)
	{
		secsize = 512;
		/* trail should be 12, but calculate it like this */
		trail = (taglen * secsize) / datalen;
	}
	else
	{
		secsize = 256 * (header[0x51] & 0x1F);
		trail = 0;
	}

	/* Allocate buffer, and ensure it's a minimum of 512 bytes */
	bufsize = secsize + trail;
	if (bufsize < 512) bufsize = 512;

	secbuf = dsk_malloc(bufsize);
	memset(secbuf, 0, bufsize);
	if (!secbuf)
	{
		dsk_free(dcself->dc42_filename);
		ldbs_close(&dcself->dc42_super.ld_store);
		fclose(fp);
		return DSK_ERR_NOMEM;
	}
	/* Attempt to load the boot sector */
	datapos = ftell(fp);
	tagpos  = datapos + datalen;
	if (fread(secbuf, 1, secsize, fp) < secsize)
	{
		dsk_free(secbuf);
		dsk_free(dcself->dc42_filename);
		ldbs_close(&dcself->dc42_super.ld_store);
		fclose(fp);
		return DSK_ERR_NOTME;
	}
	/* MacOS boot signature? */
	err = DSK_ERR_BADFMT;
	if (dg_ismacboot(secbuf))
	{
		/* Load superblock, which is at LBA 2 */
		if (fseek(fp, secsize, SEEK_CUR) ||
		    fread(secbuf, 1, secsize, fp) < secsize)
		{
			dsk_free(secbuf);
			dsk_free(dcself->dc42_filename);
			ldbs_close(&dcself->dc42_super.ld_store);
			fclose(fp);
			return DSK_ERR_NOTME;
		}
		/* Attempt to parse it */
		err = dg_hfsgeom(&geom, secbuf);	
	}
	if (err) err = dg_bootsecgeom(&geom, secbuf);
/* Can't probe geometry; assume default values */
	if (err) switch(header[0x50]) 
	{
		case 0: dg_stdformat(&geom, FMT_MAC400, NULL, NULL); break;
		case 1: dg_stdformat(&geom, FMT_MAC800, NULL, NULL); break;
		case 2: dg_stdformat(&geom, FMT_720K,   NULL, NULL); break;
		case 3: dg_stdformat(&geom, FMT_1440K,  NULL, NULL); break;
	}
	/* GCR images do include a head count in the image header; give that
	 * priority over the geometry probe. MFM images ought to (according
	 * to the spec) but I've seen at least one that doesn't:
	 * "Disk Tools_753.image". Also set GCR variant. */
	if (header[0x50] < 2)
	{
		geom.dg_heads = (header[0x51] & 0x20) ? 2 : 1;
		geom.dg_fm    = (header[0x51] & 0x1F) + RECMODE_GCR_FIRST;
	}
	for (cylinder = 0; cylinder < geom.dg_cylinders; cylinder++)
	{
		for (head = 0; head < geom.dg_heads; head++)
		{
/* Create track header */
			LDBS_TRACKHEAD *trkh;

			int secs = geom.dg_sectors;
/* On a GCR variable-speed disk, vary the number of sectors */
			if (geom.dg_fm == RECMODE_GCR_MAC) 
				secs = dg_macspt(cylinder);
	
			trkh = ldbs_trackhead_alloc(secs);
			trkh->datarate = (geom.dg_datarate == RATE_HD) ? 2 : 1;
			if (geom.dg_fm >= RECMODE_GCR_FIRST && geom.dg_fm <= RECMODE_GCR_LAST)
			{
				trkh->recmode = geom.dg_fm;
			}
			else
			{
				trkh->recmode  = 0x01;	/* MFM */
			}
			trkh->gap3     = geom.dg_fmtgap;
			trkh->filler   = 0xE5;
			trkh->total_len = 0;
/* Migrate sectors */
			for (sector = 0; sector < secs; sector++)
			{
				/* Load tags if present */
				if (trail)
				{
					if (fseek(fp, tagpos, SEEK_SET) ||
				    	    fread(secbuf + secsize, 1, 12, fp) < 12)
					{
						dsk_free(secbuf);
						dsk_free(dcself->dc42_filename);
						ldbs_close(&dcself->dc42_super.ld_store);
						fclose(fp);
						return DSK_ERR_NOTME;
					}
					tagpos = ftell(fp);
				}
				/* Load sector body */
				if (fseek(fp, datapos, SEEK_SET) ||
				    fread(secbuf, 1, secsize, fp) < secsize)
				{
					dsk_free(secbuf);
					dsk_free(dcself->dc42_filename);
					ldbs_close(&dcself->dc42_super.ld_store);
					fclose(fp);
					return DSK_ERR_NOTME;
				}
				datapos = ftell(fp);
				trkh->sector[sector].id_cyl = cylinder;
				trkh->sector[sector].id_head = head;
				trkh->sector[sector].id_sec  = sector + geom.dg_secbase;
				trkh->sector[sector].id_psh  = dsk_get_psh(secsize);
				trkh->sector[sector].st1 = 0;
				trkh->sector[sector].st2 = 0;
				trkh->sector[sector].copies = trail ? 1 : 0;
				/* Is sector blank? */
				if (!trail)
				{
					for (c = 1; c < secsize; c++)
					{
						if (secbuf[c] != secbuf[0])
						{
							trkh->sector[sector].copies = 1;
							break;
						}
					}
				}
				trkh->sector[sector].datalen = secsize;
				trkh->sector[sector].trail = trail;

				if (!trkh->sector[sector].copies)
				{
					trkh->sector[sector].filler = secbuf[0];
				}
				else	
				{
					char sectype[4];

					trkh->sector[sector].filler = trkh->filler;
					ldbs_encode_secid(sectype, 
						cylinder, head, sector + geom.dg_secbase);
					err = ldbs_putblock
						(dcself->dc42_super.ld_store,
						&trkh->sector[sector].blockid,
						sectype, secbuf, 
						secsize + trail);
				}
				if (err) break;
			}
			if (!err)
			{
				err = ldbs_put_trackhead(dcself->dc42_super.ld_store, trkh, cylinder, head);
			}
			if (trkh)
			{
				dsk_free(trkh);
				trkh = NULL;
			}
			if (err) break;
			/* End of track */
		}
		if (err) break;
		/* End of cylinder */
	}
	dsk_free(secbuf);
	if (!err) 
	{
		err = ldbs_put_geometry(dcself->dc42_super.ld_store, &geom);
	}
	if (!err && is_macbin)
	{
		long rsrcaddr, rsrclen;

		err = ldbs_putblock_d(dcself->dc42_super.ld_store, 
					DC42_MBIN_BLOCK, macbin, 128);

		if (!err)
		{
			/* Get resource fork size */
			rsrcaddr = be_peek4(macbin + 83) + 128;
			rsrcaddr = ((rsrcaddr + 127) / 128) * 128;
			rsrclen  = be_peek4(macbin + 87);
			if (rsrclen)
			{
				unsigned char *rsrcbuf = dsk_malloc(rsrclen);
				if (rsrcbuf)
				{
					if (fseek(fp, rsrcaddr, SEEK_SET) ||
				    	    fread(rsrcbuf, 1, rsrclen, fp) < rsrclen)
					{
						err = DSK_ERR_SYSERR;
					}
					if (!err)
					{
						err = ldbs_putblock_d(dcself->dc42_super.ld_store, 
							DC42_RSRC_BLOCK, rsrcbuf, rsrclen);
					}
					dsk_free(rsrcbuf);
				}
			}
		}
	}
	if (err)
	{
		dsk_free(dcself->dc42_filename);
		ldbs_close(&dcself->dc42_super.ld_store);
		fclose(fp);
		return err;
	}
	return ldbsdisk_attach(self);
}


dsk_err_t dc42_creat(DSK_DRIVER *self, const char *filename)
{
	DC42_DSK_DRIVER *dcself;
	dsk_err_t err;
	FILE *fp;
/*	unsigned char macbin[128]; */

	/* Sanity check: Is this meant for our driver? */
	if (self->dr_class != &dc_dc42) return DSK_ERR_BADPTR;
	dcself = (DC42_DSK_DRIVER *)self;

	/* Save the filename, we'll want it when doing output */
	dcself->dc42_filename = dsk_malloc_string(filename);
	if (!dcself->dc42_filename) return DSK_ERR_NOMEM;

	/* Create a 0-byte file, just to be sure we can */
	fp = fopen(filename, "wb");
	if (!fp) return DSK_ERR_SYSERR;
	fclose(fp);

	/* TODO: Should we create a MacBinary header for this new file? */

	/* OK, create a new empty blockstore */
	err = ldbs_new(&dcself->dc42_super.ld_store, NULL, LDBS_DSK_TYPE);
	if (err)
	{
		dsk_free(dcself->dc42_filename);
		return err;
	}
	/* Finally, hand the blockstore to the superclass so it can provide
	 * all the read/write/format methods */
	return ldbsdisk_attach(self);
}

typedef struct sizes
{
	size_t data_bytes;
	size_t tag_bytes;
	int recmode[RECMODE_MASK + 1];
	int datarate[4];
} SIZES;

static dsk_err_t secsize_callback(PLDBS self, dsk_pcyl_t cyl, dsk_phead_t head,
				LDBS_SECTOR_ENTRY *se, LDBS_TRACKHEAD *th,
				void *param)
{
	SIZES *siz = (SIZES *)param;

	siz->recmode[th->recmode & RECMODE_MASK]++;
	siz->datarate[th->datarate & 3]++;
	siz->data_bytes += se->datalen;
	siz->tag_bytes += se->trail;

	return DSK_ERR_OK;
}

static void cksum_buf(unsigned long *sum, unsigned char *buf, size_t len)
{
	/* This algorithm only works on an even number of bytes */
	size_t n;
	unsigned short val;

	if (len & 1) --len;
	
	for (n = 0; n < len; n += 2)
	{
		val = (buf[n] << 8) | buf[n+1];
		*sum += val;
		*sum = ((*sum & 1) ? 0x80000000 : 0) | (*sum >> 1);
	}
}


static dsk_err_t save_data(PLDBS self, dsk_pcyl_t cyl, dsk_phead_t head,
				LDBS_TRACKHEAD *th, void *param)
{
	DC42_DSK_DRIVER *dcself = (DC42_DSK_DRIVER *)param;
	dsk_err_t err;
	unsigned char *buf;
	size_t buflen;

	err = ldbs_load_track(self, th, (void **)&buf, &buflen, 0, 
			LLTO_DATA_ONLY);
	if (err) return err;

	/* Skip if no data bytes */
	if (!buflen) return DSK_ERR_OK;

	cksum_buf(&dcself->data_cksum, buf, buflen);
	if (fwrite(buf, 1, buflen, dcself->dc42_fp) < buflen)
	{
		ldbs_free(buf);
		return DSK_ERR_SYSERR;
	}
	ldbs_free(buf);
	return DSK_ERR_OK;	
}

static dsk_err_t save_tags(PLDBS self, dsk_pcyl_t cyl, dsk_phead_t head,
				LDBS_TRACKHEAD *th, void *param)
{
	DC42_DSK_DRIVER *dcself = (DC42_DSK_DRIVER *)param;
	dsk_err_t err;
	unsigned char *buf;
	size_t buflen;

	err = ldbs_load_track(self, th, (void **)&buf, &buflen, 0, 
			LLTO_TRAIL_ONLY);
	if (err) return err;
	/* Skip if no tag bytes */
	if (!buflen) return DSK_ERR_OK;

	/* We have to exclude the first 12 bytes from the checksum. If
	 * there are 12 or fewer in the buffer, that's easy - we just
	 * ignore the buffer for checksum purposes. */
	if (buflen <= dcself->tag_skip)
	{
		dcself->tag_skip -= buflen;
	}
	/* Otherwise, skip over 12 bytes and checksum the rest */
	else if (dcself->tag_skip)
	{
		cksum_buf(&dcself->tag_cksum, 
			buf + dcself->tag_skip, 
			buflen - dcself->tag_skip);
		dcself->tag_skip = 0;
	}
	else
	{
		cksum_buf(&dcself->tag_cksum, buf, buflen);
	}
	if (fwrite(buf, 1, buflen, dcself->dc42_fp) < buflen)
	{
		ldbs_free(buf);
		return DSK_ERR_SYSERR;
	}
	ldbs_free(buf);
	return DSK_ERR_OK;	

}



dsk_err_t dc42_close(DSK_DRIVER *self)
{
	DC42_DSK_DRIVER *dcself;
	LDBS_STATS stats;
	dsk_err_t err;
	unsigned char header[84];
	char *comment;
	int n, found, rmcount, recmode, drcount, datarate;
	SIZES sizes;

	/* Sanity check: Is this meant for our driver? */
	if (self->dr_class != &dc_dc42) return DSK_ERR_BADPTR;
	dcself = (DC42_DSK_DRIVER *)self;

	/* Firstly, ensure any pending changes are flushed to the LDBS 
	 * blockstore. Once this has been done we own the blockstore again 
	 * and have to close it after we've finished with it. */
	err = ldbsdisk_detach(self); 
	if (err)
	{
		dsk_free(dcself->dc42_filename);
		ldbs_close(&dcself->dc42_super.ld_store);
		return err;
	}

	/* If this disc image has not been written to, just close it and 
	 * dispose thereof. */
	if (!self->dr_dirty)
	{
		dsk_free(dcself->dc42_filename);
		return ldbs_close(&dcself->dc42_super.ld_store);
	}
	/* Trying to save changes but source is read-only */
	if (dcself->dc42_super.ld_readonly)
	{
		dsk_free(dcself->dc42_filename);
		ldbs_close(&dcself->dc42_super.ld_store);
		return DSK_ERR_RDONLY;
	}
 	/* Now write out the blockstore. Start by creating the header. */
	memset(header, 0, sizeof(header));
	if (dsk_get_comment(self, &comment) == DSK_ERR_OK && NULL != comment)
	{
		utf8_to_macroman(comment, (char *)(header + 1), 63);
	}
	else
	{
		strcpy((char *)header + 1, "-not a Macintosh disk");
	}
	header[0] = strlen((char *)header + 1);
	/* To create the header, we need to analyse the blockstore for
	 * stats. Do a second pass to analyse recording mode and 
	 * count of tag bytes, which ldbs_stats() won't pick up. */
	memset(&sizes, 0, sizeof(sizes));
	err = ldbs_get_stats(dcself->dc42_super.ld_store, &stats);
	if (!err) err = ldbs_all_sectors(dcself->dc42_super.ld_store, 
			secsize_callback, SIDES_ALT, &sizes);

	if (err)
	{
		ldbs_close(&dcself->dc42_super.ld_store);
		return err;
	}

	/* See which is the most common recording mode */
	recmode = rmcount = 0;	/* Default to 0 (unknown) */
	for (n = 0; n <= RECMODE_MASK; n++)
	{
		if (sizes.recmode[n] > rmcount) 
		{
			rmcount = sizes.recmode[n];
			recmode = n;
		}	
	}
	/* Same for data rate */
	datarate = drcount = 0;
	for (n = 0; n < 4; n++)
	{
		if (sizes.datarate[n] > drcount)
		{
			drcount = sizes.datarate[n];
			datarate = n;
		}
	}

	be_poke4(header + 0x40, sizes.data_bytes);
	be_poke4(header + 0x44, sizes.tag_bytes);
	/* Data and tag checksums can only be done after the image has been
	 * generated. */

	/* Try to determine the format, by matching the drive stats against
	 * known formats. */
	stats.drive_empty = 0;	
	for (found = n = 0; n < (int)(sizeof(known_formats) / sizeof(known_formats[0])); n++)
	{
		if (!memcmp(&stats, &known_formats[n], sizeof(stats)))
		{
			header[0x50] = n;
			found = 1;
			break;
		}
	}
	if (!found)
	{
		/* Format is not standard. Try to find the closest match */
		if (recmode >= RECMODE_GCR_FIRST && recmode <= RECMODE_GCR_LAST)
		{
			header[0x50] = (stats.max_head == 0) ? 0x00 : 0x01;
		}
		else
		{
			header[0x50] = (datarate < 2) ? 0x02 : 0x03;
		}
	}
	/* Save the GCR recording byte or MFM sector size */
	if (recmode >= RECMODE_GCR_FIRST && recmode <= RECMODE_GCR_LAST)
	{
		header[0x51] = (recmode - RECMODE_GCR_FIRST) & 0x1F;
	}
	else
	{
		header[0x51] = (stats.max_sector_size) / 256;	
	}
	if (stats.max_head > 0) header[0x51] |= 0x20;
	header[0x52] = 0x01;
	header[0x53] = 0x00;	/* Magic number */

	/* TODO: See if there's a MacBinary header and resource fork. If
	 * so, search the resource for the string ' data checksum=$' and
	 * replace the eight bytes after it with the checksum rendered 
	 * as hex. Or actually parse the thing properly, of course. */

	/* Write out the header */
	dcself->dc42_fp = fopen(dcself->dc42_filename, "wb");
	if (!dcself->dc42_fp)
	{
		ldbs_close(&dcself->dc42_super.ld_store);
		return DSK_ERR_SYSERR;
	}
	if (fwrite(header, 1, sizeof(header), dcself->dc42_fp) < (int)sizeof(header))
	{
		fclose(dcself->dc42_fp);
		remove(dcself->dc42_filename);
		ldbs_close(&dcself->dc42_super.ld_store);
		return DSK_ERR_SYSERR;
	}
	dcself->data_cksum = 0;
	dcself->tag_cksum = 0;
	dcself->tag_skip = 12;	/* The first 12 tag bytes are not skipped */
	/* Write out the data bytes */
	err = ldbs_all_tracks(dcself->dc42_super.ld_store, save_data,
				SIDES_ALT, dcself);
	if (!err) err = ldbs_all_tracks(dcself->dc42_super.ld_store, save_tags,
				SIDES_ALT, dcself);
	be_poke4(header + 0x48, dcself->data_cksum);
	be_poke4(header + 0x4C, dcself->tag_cksum);

	if (!err)	/* Rewrite the header, now with correct checksums */
	{
		if (fseek(dcself->dc42_fp, 0, SEEK_SET)) err = DSK_ERR_SYSERR;
		else if (fwrite(header, 1, sizeof(header), dcself->dc42_fp) < (int)sizeof(header)) err = DSK_ERR_SYSERR;
		else if (fclose(dcself->dc42_fp)) err = DSK_ERR_SYSERR;
	}
	if (err)
	{
		fclose(dcself->dc42_fp);
		remove(dcself->dc42_filename);
		ldbs_close(&dcself->dc42_super.ld_store);
		return err;
	}
	ldbs_close(&dcself->dc42_super.ld_store);
	return DSK_ERR_OK;
}


