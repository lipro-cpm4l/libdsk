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

/* Declarations for the DQK (compressed CPCEMU) driver */

/* The Squeeze engine (based on sq/usq by Richard Greenlaw / Theo Pozzy) */
/* Definitions and external declarations */

#define RECOGNIZE 0xFF76	/* unlikely pattern */

/* *** Stuff for first translation module *** */

#define DLE 0x90

/* *** Stuff for second translation module *** */

#define SPEOF 256	/* special endfile token */
#define NUMVALS 257	/* 256 data values plus SPEOF*/
#define NUMNODES (NUMVALS + NUMVALS - 1)        /* nbr of nodes */

#define LARGE 30000


typedef struct
{
        unsigned       dqkt_length;
        unsigned char *dqkt_data;
} DQK_TRACK;


typedef struct
{
        DSK_DRIVER dqk_super;
	char *dqk_filename;
	char *dqk_truename;
	int   dqk_readonly;
	DQK_TRACK *dqk_tracks;
	int	      dqk_ntracks;
	int	      dqk_dirty;
	dsk_psect_t   dqk_sector;	/* Last sector used for DD READ ID */
	unsigned char dqk_dskhead[256];	/* DSK header */
	unsigned char dqk_trkhead[256];	/* Current track header */
/* Internal variables used by the SQ / USQ (Huffman compression) engines */

	unsigned short crc;

	/* Decoding tree */
	struct {
	        signed short children[2];        /* left, right */
	} dnode[NUMVALS - 1];

	int bpos;        /* last bit position read */
	int curin;       /* last byte value read */

	/* Variables associated with repetition decoding */
	int repct;       /*Number of times to retirn value*/
	int value;       /*current byte value or EOF */

	/* Encoding variables */
	int likect;      /*count of consecutive identical chars */
	int lastchar, newchar;
	char state;

	int enc_track;
	int enc_offs;
	
/* The following array of structures are the nodes of the
 * binary trees. The first NUMVALS nodes becomethe leaves of the
 * final tree and represent the values of the data bytes being
 * encoded and the special endfile, SPEOF.
 * The remaining nodes become the internal nodes of the final tree.
 */

	struct   nd {
	        unsigned int weight;    /* number of appearances */
       		char tdepth;            /* length on longest path in tre*/
	        int lchild, rchild;     /* indexes to next level */
	} node[NUMNODES];

	int dctreehd;    /*index to head node of final tree */

/* This is the encoding table:
 * The bit strings have first bit in  low bit.
 * Note that counts were scaled so code fits unsigned integer
 */

	char codelen[NUMVALS];           /* number of bits in code */
	unsigned int code[NUMVALS];      /* code itself, right adjusted */
	unsigned int tcode;              /* temporary code value */


/* Variables used by encoding process */

	char cbitsrem;           /* Number of code string bits remaining */
	unsigned int ccode;      /* Current code shifted so next code bit is at right */


} DQK_DSK_DRIVER;




dsk_err_t dqk_open(DSK_DRIVER *self, const char *filename);
dsk_err_t dqk_creat(DSK_DRIVER *self, const char *filename);
dsk_err_t dqk_close(DSK_DRIVER *self);
dsk_err_t dqk_read(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
                              void *buf, dsk_pcyl_t cylinder,
                              dsk_phead_t head, dsk_psect_t sector);
dsk_err_t dqk_write(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
                              const void *buf, dsk_pcyl_t cylinder,
                              dsk_phead_t head, dsk_psect_t sector);
dsk_err_t dqk_format(DSK_DRIVER *self, DSK_GEOMETRY *geom,
                                dsk_pcyl_t cylinder, dsk_phead_t head,
                                const DSK_FORMAT *format, unsigned char filler);
dsk_err_t dqk_secid(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
                                dsk_pcyl_t cylinder, dsk_phead_t head,
                                DSK_FORMAT *result);
dsk_err_t dqk_xseek(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
                                dsk_pcyl_t cylinder, dsk_phead_t head);
dsk_err_t dqk_xread(DSK_DRIVER *self, const DSK_GEOMETRY *geom, void *buf,
                              dsk_pcyl_t cylinder, dsk_phead_t head,
                              dsk_pcyl_t cyl_expected, dsk_phead_t head_expected,
                              dsk_psect_t sector, size_t sector_size);
dsk_err_t dqk_xwrite(DSK_DRIVER *self, const DSK_GEOMETRY *geom, const void *buf,
                              dsk_pcyl_t cylinder, dsk_phead_t head,
                              dsk_pcyl_t cyl_expected, dsk_phead_t head_expected,
                              dsk_psect_t sector, size_t sector_size);
dsk_err_t dqk_status(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
                      dsk_phead_t head, unsigned char *result);




