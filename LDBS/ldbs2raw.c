/* LDBS: LibDsk Block Store access functions
 *
 *  Copyright (c) 2016,2019 John Elliott <seasip.webmaster@gmail.com>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included 
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS 
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR 
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR 
 * OTHER DEALINGS IN THE SOFTWARE. */

/* LDBS example software: 
 * Simple standalone LDBS -> raw file converter. 
 *
 * LDBS 0.4 as specified in ldbs.html
 */

#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ldbs.h"

/* Options */
int opt_verbose = 0;
int opt_trail = 0;	
dsk_sides_t opt_sidedness = SIDES_ALT;


unsigned char *offset_rec = NULL, *offset_ptr = NULL;
int any_offset = 0;

static int checkopt(const char *arg, const char *sn, const char *ln)
{
	if (!strcmp(arg, ln)     ||	/* eg --verbose */
	    !strcmp(arg, ln + 1) ||	/* eg -verbose */
	    !strcmp(arg, sn))		/* eg -v */
	{
		return 1;
	}
	return 0;
}



/* Migrate a track from LDBS to raw. */
dsk_err_t migrate_track(PLDBS infile, dsk_pcyl_t cyl, dsk_phead_t head,
	LDBS_TRACKHEAD *th, void *param)
{
	FILE *fpo = (FILE *)param;
	unsigned char *buffer;
	size_t bufsize;
	dsk_err_t err;
	
	err = ldbs_load_track(infile, th, (void **)&buffer, &bufsize, 0,
			opt_trail ? LLTO_DATA_AND_TRAIL : LLTO_DATA_ONLY);

	if (err) return err;

	fprintf(stderr, "Cyl=%d head=%d Write %d bytes at %ld\n",
		cyl, head, bufsize, ftell(fpo));	
	if (fwrite(buffer, 1, bufsize, fpo) < bufsize)
	{
		ldbs_free(buffer);
		return DSK_ERR_SYSERR;
	}
	ldbs_free(buffer);
	return DSK_ERR_OK;
}


int convert_file(const char *filename)
{
	char type[4];
	int readonly;
	char *target = malloc(10 + strlen(filename));
	char *dot;
	FILE *fpo;
	PLDBS infile;
	dsk_err_t err;

	/* Generate target filename: .ldbs -> .ufi */

	if (!target)
	{
		return DSK_ERR_NOMEM;
	}

	strcpy(target, filename);

	dot = strrchr(target, '.');
	if (dot) strcpy(dot, ".ufi");
	else	strcat(target, ".ufi");

	if (opt_verbose) printf("%s -> %s\n", filename, target);

	readonly = 1;
	err = ldbs_open(&infile, filename, type, &readonly);
	if (err) return err;

	if (opt_verbose) printf("%s opened.\n", filename);

	if (memcmp(type, LDBS_DSK_TYPE, 4))
	{
		ldbs_close(&infile);
		return DSK_ERR_NOTME;
	}

	fpo = fopen(target, "w+b");
	if (!fpo)
	{
		perror(target);
		ldbs_close(&infile);
		return DSK_ERR_SYSERR;
	}	
	/* Do the conversion */
	err = ldbs_all_tracks(infile, migrate_track, opt_sidedness, fpo);
	if (err)
	{
		fclose(fpo); 
		ldbs_close(&infile);	
		remove(target);
		free(target); 
		return err; 
	}
	if (opt_verbose) printf("Complete.\n");


/* Clean up and shut down */
	if (fclose(fpo))
	{
		perror(target);
	}
	err = ldbs_close(&infile);
	free(target);
	return err;
}



int main(int argc, char **argv)
{
	int n;
	dsk_err_t err;

	if (argc < 2)
	{
		fprintf(stderr, "%s: Syntax is %s {options} <ldbsfile> <ldbsfile> ...\n\n",
				argv[0], argv[0]);
		fprintf(stderr, "Options are: \n"
				" -v or --verbose: Verbose output\n"
				" -t or --trail:   Include trailing / metadata bytes\n"
				" -sa or --sides=alt: Process tracks in alternating-sides order\n"
				" -sb or --sides=outback: Process tracks in out-and-back order\n"
				" -so or --sides=outout: Process tracks in out-and-out order\n");
		exit(1);
	}
	for (n = 1; n < argc; n++)
	{
		if (checkopt(argv[n], "--verbose", "-v"))
		{
			opt_verbose = 1;
			continue;
		}
		if (checkopt(argv[n], "--trail", "-t"))
		{
			opt_trail = 1;
			continue;
		}
		if (checkopt(argv[n], "--sides=alt", "-sa"))
		{
			opt_sidedness = SIDES_ALT;
			continue;
		}
		if (checkopt(argv[n], "--sides=outback", "-sb"))
		{
			opt_sidedness = SIDES_OUTBACK;
			continue;
		}
		if (checkopt(argv[n], "--sides=outout", "-so"))
		{
			opt_sidedness = SIDES_OUTOUT;
			continue;
		}

		switch (err = convert_file(argv[n]))
		{
			case DSK_ERR_OK:
				break;
			case DSK_ERR_NOTME: 
				fprintf(stderr, "%s: File is not in LDBS disk format\n", argv[n]); 
				break;
			case DSK_ERR_SYSERR:
				perror(argv[n]);
				break;
			case DSK_ERR_NOMEM:
				fprintf(stderr, "Out of memory\n");
				break;
			default:
				fprintf(stderr, "%s: LibDsk error %d\n", 
					argv[n], err);
				break;
		}
	}
	return 0;
}
