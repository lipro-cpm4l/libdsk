/***************************************************************************
 *                                                                         *
 *    LIBDSK: General floppy and diskimage access library                  *
 *    Copyright (C) 2001-2  John Elliott <jce@seasip.demon.co.uk>          *
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

#include "drvi.h"

/* Standard disc geometries. These are used 
 * (i)  when logging in a disc, if the superblock is recognised
 * (ii) when formatting */

typedef struct
{
	dsk_cchar_t name;
	DSK_GEOMETRY dg;
	dsk_cchar_t desc;
} DSK_NAMEDGEOM;

static DSK_NAMEDGEOM stdg[] = 
{
		/*    sidedness cyl hd sec  psn  sz   rate    rwgap  fmtgap  fm  nomulti*/
{"pcw180",	{ SIDES_ALT,     40, 1, 9,    1, 512, RATE_SD, 0x2A, 0x52,   0,  0 }, "PCW / IBM 180k" }, /* 180k */
{"cpcsys",	{ SIDES_ALT,     40, 1, 9, 0x41, 512, RATE_SD, 0x2A, 0x52,   0,  0 }, "CPC System" }, /* CPC system */
{"cpcdata",	{ SIDES_ALT,     40, 1, 9, 0xC1, 512, RATE_SD, 0x2A, 0x52,   0,  0 }, "CPC Data" }, /* CPC data */
{"pcw720",	{ SIDES_ALT,     80, 2, 9,    1, 512, RATE_SD, 0x2A, 0x52,   0,  0 }, "PCW / IBM 720k" }, /* 720k */
{"pcw1440",	{ SIDES_ALT,     80, 2,18,    1, 512, RATE_HD, 0x1B, 0x54,   0,  0 }, "PcW16 / IBM 1440k "}, /* 1.4M */
{"ibm160",	{ SIDES_ALT,     40, 1, 8,    1, 512, RATE_SD, 0x2A, 0x50,   0,  0 }, "IBM 160k (CP/M-86 / DOSPLUS)" }, /* 160k */
{"ibm320",	{ SIDES_ALT,     40, 2, 8,    1, 512, RATE_SD, 0x2A, 0x50,   0,  0 }, "IBM 320k (CP/M-86 / DOSPLUS)" }, /* 320k */
{"ibm360",	{ SIDES_ALT,     40, 2, 9,    1, 512, RATE_SD, 0x2A, 0x52,   0,  0 }, "IBM 360k (DOSPLUS)", }, /* 360k */
{"ibm720",	{ SIDES_OUTBACK, 80, 2, 9,    1, 512, RATE_SD, 0x2A, 0x52,   0,  0 }, "IBM 720k (144FEAT)", }, /* 720k 144FEAT */
{"ibm1200",	{ SIDES_OUTBACK, 80, 2,15,    1, 512, RATE_HD, 0x1B, 0x54,   0,  0 }, "IBM 1.2M (144FEAT)", }, /* 1.2M 144FEAT */
{"ibm1440",	{ SIDES_OUTBACK, 80, 2,18,    1, 512, RATE_HD, 0x1B, 0x54,   0,  0 }, "IBM 1.4M (144FEAT)", }, /* 1.4M 144FEAT */
{"acorn160",	{ SIDES_OUTOUT,  40, 1,16,    0, 256, RATE_SD, 0x12, 0x60,   0,  0 }, "Acorn 160k" }, /* Acorn 160k */
{"acorn320",	{ SIDES_OUTOUT,  80, 1,16,    0, 256, RATE_SD, 0x12, 0x60,   0,  0 }, "Acorn 320k" }, /* Acorn 320k */
{"acorn640",	{ SIDES_OUTOUT,  80, 2,16,    0, 256, RATE_SD, 0x12, 0x60,   0,  0 }, "Acorn 640k" }, /* Acorn 640k */
{"acorn800",	{ SIDES_ALT,     80, 2, 5,    0,1024, RATE_SD, 0x04, 0x05,   0,  0 }, "Acorn 800k" }, /* Acorn 800k */
{"acorn1600",	{ SIDES_ALT,     80, 2,10,    0,1024, RATE_HD, 0x04, 0x05,   0,  0 }, "Acorn 1600k" }, /* Acorn 1600k */
{"pcw800",	{ SIDES_ALT,     80, 2,10,    1, 512, RATE_SD, 0x0C, 0x17,   0,  0 }, "PCW 800k" }, /* 800k */
{"pcw200",	{ SIDES_ALT,     40, 1,10,    1, 512, RATE_SD, 0x0C, 0x17,   0,  0 }, "PCW 200k" }, /* 200k */
};

/* Initialise a DSK_GEOMETRY with a standard format */
dsk_err_t dg_stdformat(DSK_GEOMETRY *self, dsk_format_t formatid,
			dsk_cchar_t *fname, dsk_cchar_t *fdesc)
{
	int idx = (formatid - FMT_180K);

	if (idx >= (sizeof(stdg)/sizeof(stdg[0]))  ) return DSK_ERR_BADFMT;
	if (self) memcpy(self, &stdg[idx].dg, sizeof(*self));
	if (fname) *fname = stdg[idx].name;
	if (fdesc) *fdesc = stdg[idx].desc;
	return DSK_ERR_OK;
}



