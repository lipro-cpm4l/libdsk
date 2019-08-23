/***************************************************************************
 *                                                                         *
 *    LIBDSK: General floppy and diskimage access library                  *
 *    Copyright (C) 2002, 2019  John Elliott <seasip.webmaster@gmail.com>  *
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

/* Compaq QRST5 decompression. Converts QRST5 (compressed with the PKWARE 
 * Data Compression Library 'implode' compression) to plain QRST.
 *
 */

#include "compi.h"
#include "compqrst.h"
#include "ldbs.h"	/* for ldbs_peek4 */

/* This struct contains function pointers to the driver's functions, and the
 * size of its DSK_DRIVER subclass */

COMPRESS_CLASS cc_qrst5 = 
{
	sizeof(QRST5_COMPRESS_DATA),
	"qrst5",
	"QRST (v5) compression",
	qrst5_open,	/* open */
	qrst5_creat,	/* create new */
	qrst5_commit,	/* commit */
	qrst5_abort	/* abort */
};

#include "blast.h"

static unsigned blast_input(void *how, unsigned char **buf)
{
	QRST5_COMPRESS_DATA *qrst5_self = (QRST5_COMPRESS_DATA *)how;
	int len;

	len = fread(qrst5_self->inbuf, 1, sizeof(qrst5_self->inbuf),
			qrst5_self->fp_in);
	*buf = qrst5_self->inbuf;
	return len;	
}

static int blast_output(void *how, unsigned char *buf, unsigned len)
{
	QRST5_COMPRESS_DATA *qrst5_self = (QRST5_COMPRESS_DATA *)how;

	while (len)
	{
/* To convert from QRST5 format to QRST format, we have to inject a track 
 * header into the byte stream every <track_length> bytes */
		if (!qrst5_self->tracklen)	
		{
			if (fputc(qrst5_self->cylinder, qrst5_self->fp_out) == EOF) return 1;
			if (fputc(qrst5_self->head, qrst5_self->fp_out) == EOF) return 1;
			if (fputc(0x00, qrst5_self->fp_out) == EOF) return 1;
			qrst5_self->tracklen = qrst5_self->geom.dg_secsize *
					       qrst5_self->geom.dg_sectors;
			/* Increase cylinder and head */
			++qrst5_self->head;
			if (qrst5_self->head >= qrst5_self->geom.dg_heads)
			{
				++qrst5_self->cylinder;
				qrst5_self->head = 0;
			}
		}
		if (fputc(*buf, qrst5_self->fp_out) == EOF) return 1;
		++buf;
		--len;
		--qrst5_self->tracklen;
	}
	return 0;
}


static int blast_getgeom(void *how, unsigned char *buf, unsigned len)
{
	QRST5_COMPRESS_DATA *qrst5_self = (QRST5_COMPRESS_DATA *)how;

	/* Fill the bootsector buffer */
	if (qrst5_self->bootsec_len < 512)
	{
		int l = 512 - qrst5_self->bootsec_len;
		if (l > len) l = len;

		memcpy(qrst5_self->bootsec + qrst5_self->bootsec_len, buf, l);
		qrst5_self->bootsec_len += l;

		/* If it's full, stop */
		if (qrst5_self->bootsec_len >= 512) return 1;
	}
	return 0;
}

/* Disk capacity from QRST header */
static dsk_format_t qrst_format[] =
{
	FMT_UNKNOWN,
	FMT_360K,	
	FMT_1200K,
	FMT_720K,
	FMT_1440K,
	FMT_160K,
	FMT_180K,
	FMT_320K
};


static dsk_err_t decompress(QRST5_COMPRESS_DATA *qrst5_self, long offset)
{
	/* We decompress twice. First time we just pull out the boot 
	 * sector */
	if (fseek(qrst5_self->fp_in, offset, SEEK_SET)) return DSK_ERR_SYSERR;

	qrst5_self->bootsec_len = 0;
	memset(qrst5_self->bootsec, 0, sizeof(qrst5_self->bootsec));
	memset(&qrst5_self->geom, 0, sizeof(qrst5_self->geom));
	if (blast(blast_input, qrst5_self,
		  blast_getgeom, qrst5_self, NULL, NULL) < 0)
	{
		return DSK_ERR_COMPRESS;
	}

	if (fseek(qrst5_self->fp_in, offset, SEEK_SET)) return DSK_ERR_SYSERR;

	if (qrst5_self->bootsec_len < 512 || dg_bootsecgeom(&qrst5_self->geom, qrst5_self->bootsec))
	{
		/* Geometry probe failed. Fall back on drive type in header */
		dg_stdformat(&qrst5_self->geom, qrst_format[qrst5_self->header[12]], NULL, NULL);
	}
	/* Initialise the decompression state machine */
	qrst5_self->cylinder = 0;
	qrst5_self->head = 0;
	qrst5_self->tracklen = 0;


	/* And now decompress for real */

	switch (blast(blast_input, qrst5_self,
		      blast_output, qrst5_self, NULL, NULL))
	{
		case 0: return DSK_ERR_OK;
		case 1:
		case 2: return DSK_ERR_SYSERR;
		default:
			return DSK_ERR_COMPRESS;
	}
}



dsk_err_t qrst5_open(COMPRESS_DATA *self)
{
	QRST5_COMPRESS_DATA *qrst5_self;
	dsk_err_t err;
	unsigned long offset;
	
	/* Sanity check: Is this meant for our driver? */
	if (self->cd_class != &cc_qrst5) return DSK_ERR_BADPTR;
	qrst5_self = (QRST5_COMPRESS_DATA *)self;
	qrst5_self->fp_in = NULL;
	qrst5_self->fp_out = NULL; 

	/* Open the file to decompress */
	err = comp_fopen(self, &qrst5_self->fp_in);
	if (err) return DSK_ERR_NOTME;

	/* Check for QRST5 signature */
	if (fread(qrst5_self->header, 1, sizeof(qrst5_self->header), 
		qrst5_self->fp_in) < (int)sizeof(qrst5_self->header) || 
		memcmp(qrst5_self->header, "QRST", 4) ||
		qrst5_self->header[12] < 1 || 
		qrst5_self->header[12] > 7 || 
		qrst5_self->header[795] != 2)
	{
		fclose(qrst5_self->fp_in);
		return DSK_ERR_NOTME;	
	}
	/* OK. This is a compressed QRST file. */
	err = comp_mktemp(self, &qrst5_self->fp_out);

	if (!err)
	{
		offset = ldbs_peek4(qrst5_self->header + 797);
/* Write an old-style QRST header */
		qrst5_self->header[795] = 0;
		if (fwrite(qrst5_self->header, 1, 796, qrst5_self->fp_out) < 796)
		{
			err = DSK_ERR_SYSERR;			
		}		
		if (!err) err = decompress(qrst5_self, offset);
	}	
	fclose(qrst5_self->fp_in);
	if (qrst5_self->fp_out) fclose(qrst5_self->fp_out);
/* This is a decompress-only driver, so set the read-only flag in the 
 * compression structure */
	self->cd_readonly = 1;
		
	return err;	
}


dsk_err_t qrst5_commit(COMPRESS_DATA *self)
{
	return DSK_ERR_RDONLY;

/*	This is where compression would be implemented 

	QRST5_COMPRESS_DATA *qrst5_self;
	dsk_err_t err = DSK_ERR_OK;
	
	if (self->cd_class != &cc_qrst5) return DSK_ERR_BADPTR;
	qrst5_self = (QRST5_COMPRESS_DATA *)self;

	qrst5_self->fp_in = NULL;
	qrst5_self->fp_out = NULL;
        if (self->cd_cfilename && self->cd_ufilename)
        {
		qrst5_self->fp_in  = fopen(self->cd_ufilename, "rb");
		qrst5_self->fp_out = fopen(self->cd_cfilename, "wb");
		if (!qrst5_self->fp_in || !qrst5_self->fp_out) err = DSK_ERR_SYSERR;
		else err = compress(qrst5_self);
	}
	if (qrst5_self->fp_in) fclose(qrst5_self->fp_in);
	if (qrst5_self->fp_out) fclose(qrst5_self->fp_out);

	return err;
*/
}


/* abort */
dsk_err_t qrst5_abort(COMPRESS_DATA *self)
{
/*	QRST5_COMPRESS_DATA *qrst5_self;
	if (self->cd_class != &cc_qrst5) return DSK_ERR_BADPTR;
	qrst5_self = (QRST5_COMPRESS_DATA *)self; */

	return DSK_ERR_OK;
}

/* Create */
dsk_err_t qrst5_creat(COMPRESS_DATA *self)
{
	return DSK_ERR_RDONLY;
}



