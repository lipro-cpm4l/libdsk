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

/* Simple wrapper around dsk_getgeom() */

#include <stdio.h>
#include "libdsk.h"
#include "utilopts.h"

#ifdef CPM
#define AV0 "DSKID"
#else
#define AV0 argv[0]
#endif

int do_login(char *outfile, char *outtyp, int forcehead);

int main(int argc, char **argv)
{
	char *outtyp;
	int forcehead;

	if (argc < 2)
	{
		fprintf(stderr, "Syntax: \n"
                        "      %s dskimage { -type <type> } { -side <side> }\n",
			AV0);
		fprintf(stderr,"\nDefault type is autodetect.\n\n");
		
		fprintf(stderr, "eg: %s myfile.DSK\n"
                                "    %s /dev/fd0 -type floppy -side 1\n", AV0, AV0);
		return 1;
	}
	outtyp    = check_type("-type", argc, argv);
	forcehead = check_forcehead("-side", argc, argv);	

	return do_login(argv[1], outtyp, forcehead);
}



int do_login(char *outfile, char *outtyp, int forcehead)
{
	DSK_DRIVER *outdr = NULL;
	dsk_err_t e;
	DSK_GEOMETRY dg;
	unsigned char drv_status;

	e = dsk_open(&outdr, outfile, outtyp);
	if (!e) e = dsk_set_forcehead(outdr, forcehead);
	if (!e) e = dsk_getgeom(outdr, &dg);
	if (!e)
	{
                printf("Driver: %s\n", dsk_drvdesc(outdr));

		if (forcehead >= 0) printf("[Forced to read from side %d]\n", forcehead);
		printf("Sidedness:     %2d\n"
                       "Cylinders:     %2d\n"
		       "Heads:          %d\n"
                       "Sectors:      %3d\n"
                       "First sector: %3d\n"
                       "Sector size: %4d\n"
		       "Data rate:      %d\n"
		       "R/W gap:     0x%02x\n"
		       "Format gap:  0x%02x\n",
			dg.dg_sidedness, dg.dg_cylinders,
			dg.dg_heads, dg.dg_sectors, dg.dg_secbase,
			dg.dg_secsize, dg.dg_datarate,
			dg.dg_rwgap,   dg.dg_fmtgap);
		e = dsk_drive_status(outdr, &dg, 0, &drv_status);
		if (!e)  printf("\nDrive status:  0x%02x\n", drv_status);
	}
	if (outdr) dsk_close(&outdr);
	if (e)
	{
		fprintf(stderr, "%s\n", dsk_strerror(e));
		return 1;
	}
	return 0;
}

