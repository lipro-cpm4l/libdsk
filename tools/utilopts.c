/***************************************************************************
 *                                                                         *
 *    LIBDSK: General floppy and diskimage access library                  *
 *    Copyright (C) 2001,2019  John Elliott <seasip.webmaster@gmail.com>   *
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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "config.h"
#include "utilopts.h"
#include "libdsk.h"
#ifdef HAVE_WINDOWS_H
#include <windows.h>
# ifdef HAVE_WINIOCTL_H
#  include <winioctl.h>
# endif
# define FDRAWCMD_VERSION		0x0100000f
# define IOCTL_FD_BASE                   FILE_DEVICE_UNKNOWN
# define IOCTL_FDRAWCMD_GET_VERSION	CTL_CODE(IOCTL_FD_BASE, 0x888, METHOD_BUFFERED, FILE_READ_DATA|FILE_WRITE_DATA)
#endif

int version(void)
{
	printf("libdsk version %s\n", LIBDSK_VERSION);
	return 0;
}

int types(void)
{
	int n = 0;
	char *type, *desc;
	printf("Disk image types supported:\n\n");

	while (!dsk_type_enum(n, &type)) 
	{
		dsk_typedesc_enum(n++, &desc);
                printf("   %-10.10s : %s\n", type, desc);
	}
	return 0;
}

int formats(void)
{
        dsk_format_t format;
        dsk_cchar_t fname, fdesc;

	printf("Disk formats supported:\n\n");

        format = FMT_180K;
        while (dg_stdformat(NULL, format++, &fname, &fdesc) == DSK_ERR_OK)
        {
                printf("   %-10.10s : %s\n", fname, fdesc);
        }
	return 0;
}


int standard_args(int argc, char **argv) 
{
        if (find_arg("--version", argc, argv) > 0 ||
            find_arg("-version", argc, argv) > 0) return version();
        if (find_arg("--formats", argc, argv) > 0 ||
            find_arg("-formats", argc, argv) > 0) return formats();
        if (find_arg("--types", argc, argv) > 0 ||
            find_arg("-types", argc, argv) > 0) return types();
	return -1;
}


void excise_arg(int na, int *argc, char **argv)
{	
	int n;

	--(*argc);
	for (n = na; n < *argc; n++)
	{
		argv[n] = argv[n+1];
	}
}

int find_arg(char *arg, int argc, char **argv)
{
	int n;
	
	for (n = 1; n < argc; n++)
	{
		if (!strcmpi(argv[n], "--")) return -1;
		if (!strcmpi(argv[n], arg)) return n;
	}
	return -1;
}


int check_forcehead(char *arg, int *argc, char **argv)
{
	int n = find_arg(arg, *argc, argv);
	int fh;

	if (n < 0) return -1;	
	/* Remove the "-head" */
	excise_arg(n, argc, argv);
	if (n >= *argc || argv[n][0] < '0' || argv[n][0] > '1') 
	{
		fprintf(stderr, "Syntax error: use '%s 0' or '%s 1'\n", arg, arg);
		exit(1);
	}
	fh = argv[n][0] - '0';
	excise_arg(n, argc, argv);

	return fh;
}


unsigned check_retry(char *arg, int *argc, char **argv)
{
	int n = find_arg(arg, *argc, argv);
	unsigned nr;

	if (n < 0) return 1;	
	/* Remove the "-retryy" */
	excise_arg(n, argc, argv);
	if (n >= *argc || atoi(argv[n]) == 0)
	{
		fprintf(stderr, "Syntax error: use '%s nnn' where nnn is nonzero\n", arg);
		exit(1);
	}
	nr = atoi(argv[n]);
	excise_arg(n, argc, argv);

	return nr;
}

static char *st_cmt = NULL;
static unsigned st_cmt_size = 0;

static void append_comment(char *s)
{
	if (st_cmt == NULL)
	{
		st_cmt = malloc(1024);
		if (!st_cmt)
		{
			fprintf(stderr, "Out of memory entering comment");
			exit(1);
		}
		st_cmt_size = 1024;
		st_cmt[0] = 0;
	}
	while (strlen(st_cmt) + strlen(s) > st_cmt_size)
	{
		st_cmt = realloc(st_cmt, 2 * st_cmt_size);
		if (!st_cmt)
		{
			fprintf(stderr, "Out of memory entering comment");
			exit(1);
		}
		st_cmt_size *= 2;
	}
	strcat(st_cmt, s);
}


static char *get_comment_stdin(void)
{
	char buf[81];
	char *p;

	printf("Enter the comment. Use a dot on a line by itself to mark the "
		"end.\n");
	do
	{
		putchar(':');
		fflush(stdout);
		p = fgets(buf, sizeof(buf), stdin);
		if ((p == NULL) || !strcmp(p, ".") || !strcmp(p, ".\n"))
			break;
		append_comment(p);
	} while (1);
	
	return st_cmt;
}

static char *get_comment(char *filename)
{
	char buf[81];
	FILE *fp = fopen(filename, "r");

	if (!fp)
	{
		perror(filename);
		exit(1);
	}
	while (fgets(buf, sizeof(buf), fp))
	{
		append_comment(buf);
	}
	fclose(fp);
	return st_cmt;
}

char *check_comment(char *arg, int *argc, char **argv)
{
	int n = find_arg(arg, *argc, argv);
	char *v;

	if (n < 0) return NULL;
	/* Remove the "-comment" */
	excise_arg(n, argc, argv);
	if (n >= *argc)
	{
		return get_comment_stdin();
	}
	v = argv[n];
	excise_arg(n, argc, argv);
	if (v[0] == '@')
	{
		if (v[1] == '-') return get_comment_stdin();
		return get_comment(v + 1);
	}
	return v;
}


char *check_type(char *arg, int *argc, char **argv)
{
	int n = find_arg(arg, *argc, argv);
	char *v;

	if (n < 0) return NULL;
	/* Remove the "-type" */
	excise_arg(n, argc, argv);
	if (n >= *argc)
	{
		fprintf(stderr, "Syntax error: use '%s <type>'\n", arg);
		exit(1);
	}
	v = argv[n];
	/* Remove the argument */
	excise_arg(n, argc, argv);

	if (!strcmpi(v, "auto")) return NULL;
	return v;
}


int present_arg(char *arg, int *argc, char **argv)
{
	int n = find_arg(arg, *argc, argv);

	if (n < 0) return 0;

	excise_arg(n, argc, argv);
	return 1;
}


int ignore_arg(char *arg, int count, int *argc, char **argv)
{
	int n = find_arg(arg, *argc, argv);

	if (n < 0) return 0;

	fprintf(stderr, "Warning: option '");
	while (count)
	{
		fprintf(stderr, "%s ", argv[n]);
		excise_arg(n, argc, argv);
		--count;
		if (n >= *argc) break;	
	}
	fprintf(stderr, "\b' ignored.\n");
	return 1;
}

void args_complete(int *argc, char **argv)
{
	int n;
	for (n = 1; n < *argc; n++)
	{
		if (!strcmpi(argv[n], "--")) 
		{
			excise_arg(n, argc, argv);
			return;
		}
		if (argv[n][0] == '-')
		{
			fprintf(stderr, "Unknown option: %s\n", argv[n]);
			exit(1);
		}
	}
}

static const char *known_exts[] =
{
	"td0", "tele",		/* Teledisk */
	"cqm", "copyqm",	/* CopyQM */
	"ldbs", "ldbs",		/* LDBS */
	"cfi", "cfi",		/* CFI */
	"ufi", "rawalt",	/* Raw floppy */
	"floppyimage", "rawalt",/* Raw floppy */
	"vfd", "rawalt",	/* VirtualBox virtual floppy */
	"ssd", "rawalt",	/* BBC Micro single-sided */
	"dsd", "rawalt",	/* BBC Micro double-sided */
	"imd", "imd",		/* IMD */
	NULL, NULL
};


const char *guess_type(const char *arg)
{
	const char *ext;
	int n;

/* Attempt to guess the output file type based on the filename */
	if (!strncmp(arg, "gotek:", 6) ||
	    !strncmp(arg, "gotek144:", 9) ||
	    !strncmp(arg, "gotek1440:", 10)) return "gotek1440";
	if (!strncmp(arg, "gotek72:", 8) ||
	    !strncmp(arg, "gotek720:", 9)) return "gotek720";

#ifdef HAVE_LINUX_FDREG_H
	if (!strncmp(arg, "/dev/fd", 7)) return "floppy";
#endif

#ifdef HAVE_WINDOWS_H
	if (strlen(arg) == 2 && arg[1] == ':')
	{
/* See if ntwdm driver is present */
		HANDLE hDriver;
		DWORD dwVersion, dwRet;
		BOOL ret;

		hDriver = CreateFile("\\\\.\\fdrawcmd", 
			GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ, 
			NULL, OPEN_EXISTING, 0, NULL);
		if (hDriver == INVALID_HANDLE_VALUE) return "floppy";

		ret = DeviceIoControl(hDriver, IOCTL_FDRAWCMD_GET_VERSION,
			NULL, 0, &dwVersion, sizeof(dwVersion), &dwRet, NULL);
	
		CloseHandle(hDriver);
		if (!ret || (dwVersion ^ FDRAWCMD_VERSION) & 0xffff0000)
			return "floppy";
		return "ntwdm";
	}
#endif

#ifdef __MSDOS__
	if (strlen(arg) == 2 && arg[1] == ':') return "floppy";
#endif

	if (!strncmp(arg, "fork:", 5) || !strncmp(arg, "serial:", 7))
		return "remote";

	/* Check for known file extensions */
	ext = strrchr(arg, '.');
	if (!ext) return "dsk";
	++ext;
	for (n = 0; known_exts[n]; n += 2)
	{
		if (!strcmpi(ext, known_exts[n])) return known_exts[n + 1];
	}
	/* All else fails, default to dsk */
	return "dsk";
}
