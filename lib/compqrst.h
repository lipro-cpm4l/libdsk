/***************************************************************************
 *                                                                         *
 *    LIBDSK: General floppy and diskimage access library                  *
 *    Copyright (C) 2002,2019  John Elliott <seasip.webmaster@gmail.com>   *
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

/* Compaq QRST (v5) decompression */

typedef struct
{
        COMPRESS_DATA qrst5_super;
	unsigned char inbuf[1024];
	unsigned char bootsec[512];
	unsigned char header[821];
	unsigned bootsec_len;
	DSK_GEOMETRY geom;
	dsk_pcyl_t cylinder;
	dsk_phead_t head;
	unsigned tracklen;
	
	FILE *fp_in;
	FILE *fp_out;
} QRST5_COMPRESS_DATA;


extern COMPRESS_CLASS cc_qrst5;


dsk_err_t qrst5_open(COMPRESS_DATA *self);
dsk_err_t qrst5_creat(COMPRESS_DATA *self);
dsk_err_t qrst5_commit(COMPRESS_DATA *self);
dsk_err_t qrst5_abort(COMPRESS_DATA *self);

