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

/* Example formatter */

#include <stdio.h>
#include <string.h>
#include "config.h"
#include "libdsk.h"
#include "utilopts.h"
#include "formname.h"

#ifdef CPM
#define AV0 "DSKFORM"
#else
#define AV0 argv[0]
#endif


int do_format(char *outfile, char *outtyp, char *outcomp, int forcehead, dsk_format_t format);
dsk_format_t check_format(char *arg, int argc, char **argv);

int help(int argc, char **argv)
{
	fprintf(stderr, "Syntax: \n"
                "      %s dskimage { -format <format> } "
                " { -type <type> } { -side <side> }\n",
		AV0);
	fprintf(stderr,"\nDefault type is DSK.\nDefault format is PCW 180k.\n\n");
		
	fprintf(stderr, "eg: %s myfile.DSK\n"
                        "    %s /dev/fd0 -type floppy -format cpcsys -side 1\n", AV0, AV0);

	valid_formats();
	return 1;
}



int main(int argc, char **argv)
{
	char *outtyp;
	char *outcomp;
	int forcehead;
	dsk_format_t format;

	if (argc < 2) return help(argc, argv);
        if (find_arg("--help",    argc, argv) > 0) return help(argc, argv);
        if (find_arg("--version", argc, argv) > 0) return version();
        outtyp    = check_type("-type", argc, argv); if (!outtyp) outtyp = "dsk";
        outcomp   = check_type("-comp", argc, argv); 
        forcehead = check_forcehead("-side", argc, argv);
	format    = check_format("-format", argc, argv);
	if (format == -1) format = FMT_180K;

	return do_format(argv[1], outtyp, outcomp, forcehead, format);
}

static unsigned char spec169 [10] = { 0,    0, 40, 9, 2, 2, 3, 2, 0x2A, 0x52 };
static unsigned char spec180 [10] = { 0,    0, 40, 9, 2, 1, 3, 2, 0x2A, 0x52 };
static unsigned char spec200 [10] = { 0,    0, 40,10, 2, 1, 3, 3, 0x0C, 0x17 };
static unsigned char spec720 [10] = { 3, 0x81, 80, 9, 2, 1, 4, 4, 0x2A, 0x52 };
static unsigned char spec800 [10] = { 3, 0x81, 80,10, 2, 1, 4, 4, 0x0C, 0x17 };

int do_format(char *outfile, char *outtyp, char *outcomp, int forcehead, dsk_format_t format)
{
	DSK_PDRIVER outdr = NULL;
	dsk_err_t e;
	dsk_pcyl_t cyl;
	dsk_phead_t head;
	DSK_GEOMETRY dg;
	dsk_cchar_t fdesc;

	e = dsk_creat(&outdr, outfile, outtyp, outcomp);
	if (!e) e = dsk_set_forcehead(outdr, forcehead);
	if (!e) e = dg_stdformat(&dg, format, NULL, &fdesc);
	if (!e)
	{
		printf("Formatting %s as %s\n", outfile, fdesc);
		for (cyl = 0; cyl < dg.dg_cylinders; cyl++)
		{
		    for (head = 0; head < dg.dg_heads; head++)
		    {
			printf("Cyl %02d/%02d Head %d/%d\r", 
				cyl +1, dg.dg_cylinders,
			 	head+1, dg.dg_heads);
			fflush(stdout);

			if (!e) e = dsk_apform(outdr, &dg, cyl, head, 0xE5);
			if (e) break;	
		    }
		}
	
	}
	/* Create a disc spec on the first sector, if the format's recognised */
	if (!e)
	{
		unsigned char bootsec[512];
		unsigned int do_bootsec = 0;

		memset(bootsec, 0xE5, sizeof(bootsec));
		switch(format)
		{
			case FMT_180K:
				memcpy(bootsec, spec180, 10);
				do_bootsec = 1;
				break;
			case FMT_CPCSYS:
				memcpy(bootsec, spec169, 10);
				do_bootsec = 1;
				break;
			case FMT_CPCDATA:
				break;
			case FMT_200K:
				memcpy(bootsec, spec200, 10);
				do_bootsec = 1;
				break;
			case FMT_720K:
				memcpy(bootsec, spec720, 10);
				do_bootsec = 1;
				break;
			case FMT_800K:
				memcpy(bootsec, spec800, 10);
				do_bootsec = 1;
				break;
			default:
				break;
		}
		if (do_bootsec) e = dsk_lwrite(outdr, &dg, bootsec, 0);
	}
	printf("\r                                     \r");
	if (outdr) dsk_close(&outdr);
	if (e)
	{
		fprintf(stderr, "%s\n", dsk_strerror(e));
		return 1;
	}
	return 0;
}


