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

/* Portable equivalent of PCWTRANS */

#include <stdio.h>
#include <stdlib.h>
#include "libdsk.h"
#include "utilopts.h"
#include "formname.h"
#include <errno.h>

#ifdef CPM
#define AV0 "DSKTRANS"
#else
#define AV0 argv[0]
#endif

static int md3 = 0;
static int logical = 0;
static dsk_format_t format = -1;
static	char *intyp = NULL, *outtyp = NULL;
static	char *incomp = NULL, *outcomp = NULL;
static	int inside = -1, outside = -1;

int do_copy(char *infile, char *outfile);


int help(int argc, char **argv)
{
	fprintf(stderr, "Syntax: \n"
                       "      %s in-image out-image { options }\n",
			AV0);
	fprintf(stderr,"\nOptions are:\n"
		       "-itype <type>   type of input disc image\n"
                       "-otype <type>   type of output disc image\n"
                       "-iside <side>   Force side 0 or side 1 of input\n"
                       "-oside <side>   Force side 0 or side 1 of output\n"
                       "-md3            Defeat MicroDesign 3 copy protection\n"
		       "-logical        Rearrange tracks in logical order\n"
                       "                (Only useful when out-image type is 'raw' and reading discs\n"
		       "                with non-IBM track layout (eg: 144FEAT 1.4Mb or ADFS 640k)\n"
		       "-format         Force a specified format name\n");
	fprintf(stderr,"\nDefault in-image type is autodetect."
		               "\nDefault out-image type is DSK.\n\n");
		
	fprintf(stderr, "eg: %s /dev/fd0 myfile1.DSK\n"
                        "    %s /dev/fd0 myfile2.DSK -side 1\n" 
                        "    %s /dev/fd0 md3boot.dsk -md3\n" 
                        "    %s myfile.DSK /dev/fd0 -otype floppy\n", 
			AV0, AV0, AV0, AV0);
	valid_formats();
	return 1;
}


int main(int argc, char **argv)
{

	if (find_arg("--version", argc, argv) > 0) return version(); 
	if (argc < 3) return help(argc, argv);
	if (find_arg("--help",    argc, argv) > 0) return help(argc, argv);
	intyp     = check_type("-itype", argc, argv); 
        outtyp    = check_type("-otype", argc, argv);
	incomp    = check_type("-icomp", argc, argv); 
        outcomp   = check_type("-ocomp", argc, argv);
        inside    = check_forcehead("-iside", argc, argv);
        outside   = check_forcehead("-oside", argc, argv);
	if (find_arg("-md3", argc, argv) > 0) md3 = 1;
	if (find_arg("-logical", argc, argv) > 0) logical = 1;
	if (!outtyp) outtyp = "dsk";
        format    = check_format("-format", argc, argv);

	return do_copy(argv[1], argv[2]);
}



int do_copy(char *infile, char *outfile)
{
	DSK_PDRIVER indr = NULL, outdr = NULL;
	dsk_err_t e;
	dsk_pcyl_t cyl;
	dsk_phead_t head;
	dsk_psect_t sec;
	char *buf = NULL;
	DSK_GEOMETRY dg;
	char *op = "Opening";

	        e = dsk_open (&indr,  infile,  intyp, incomp);
	if (!e) e = dsk_set_forcehead(indr, inside);
	if (!e) e = dsk_creat(&outdr, outfile, outtyp, outcomp);
	if (!e) e = dsk_set_forcehead(outdr, outside);
	if (format == -1)
	{
		op = "Identifying disc";
		if (!e) e = dsk_getgeom(indr, &dg);
	}
	else if (!e) e = dg_stdformat(&dg, format, NULL, NULL);
	if (!e)
	{	
		buf = dsk_malloc(dg.dg_secsize);
		if (!buf) e = DSK_ERR_NOMEM;
	}
	if (!e)
	{
		printf("Input driver: %s\nOutput driver:%s\n%s",
			dsk_drvdesc(indr), dsk_drvdesc(outdr),
			logical ? "[tracks rearranged]\n" : "");
		for (cyl = 0; cyl < dg.dg_cylinders; ++cyl)
		{
		    for (head = 0; head < dg.dg_heads; ++head)
		    {
                        if (md3)
                        {
			// MD3 discs have 256-byte sectors in cyl 1 head 1
			    if (cyl == 1 && head == 1) dg.dg_secsize = 256;
			    else		       dg.dg_secsize = 512;
                        }
			op = "Formatting";
			// Format track!
                        if (!e) e = dsk_apform(outdr, &dg, cyl, head, 0xE5);

			if (!e) for (sec = 0; sec < dg.dg_sectors; ++sec)
			{
				printf("Cyl %02d/%02d Head %d/%d Sector %03d/%03d\r", 
					cyl +1, dg.dg_cylinders,
				 	head+1, dg.dg_heads,
					sec+dg.dg_secbase, dg.dg_sectors + dg.dg_secbase - 1); 
				fflush(stdout);
			
				op = "Reading";	
				if (logical)
				{
					dsk_lsect_t ls;
					dsk_sides_t si;

	/* Convert sector to logical using SIDES_ALT sidedness. Raw DSKs 
	 * are always created so that the tracks are stored in SIDES_ALT 
	 * order. */
					si = dg.dg_sidedness;
					dg.dg_sidedness = SIDES_ALT;
					dg_ps2ls(&dg, cyl, head, sec, &ls);
					dg.dg_sidedness = si;
					e = dsk_lread(indr, &dg, buf, ls);
				}
				else e = dsk_pread(indr, &dg, buf, cyl,head,sec + dg.dg_secbase);
				// MD3 discs have deliberate bad sectors in cyl 1 head 1
                                if (md3 && e == DSK_ERR_DATAERR && dg.dg_secsize == 256) e = DSK_ERR_OK;
				if (e) break;
				op = "Writing";
				e = dsk_pwrite(outdr,&dg,buf,cyl,head, sec + dg.dg_secbase);
				if (e) break;
			}
			if (e) break;
		    }
		    if (e) break;
		}
	
	}
	printf("\r                                     \r");
	if (indr)  dsk_close(&indr);
	if (outdr) dsk_close(&outdr);
	if (buf) dsk_free(buf);
	if (e)
	{
		fprintf(stderr, "\n%s: %s\n", op, dsk_strerror(e));
		return 1;
	}
	return 0;
}

