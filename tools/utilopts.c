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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "config.h"
#include "utilopts.h"
#include "libdsk.h"

int version(void)
{
	printf("libdsk version %s\n", LIBDSK_VERSION);
	return 0;
}

int find_arg(char *arg, int argc, char **argv)
{
	int n;
	
	for (n = 1; n < argc; n++)
	{
		if (!strcmpi(argv[n], arg)) return n;
	}
	return -1;
}


int check_forcehead(char *arg, int argc, char **argv)
{
	int n = find_arg(arg, argc, argv);

	if (n < 0) return -1;	
	++n;
	if (n >= argc || argv[n][0] < '0' || argv[n][0] > '1') 
	{
		fprintf(stderr, "Syntax error: use '%s 0' or '%s 1'\n", arg, arg);
		exit(1);
	}
	return argv[n][0] - '0';	
}

char *check_type(char *arg, int argc, char **argv)
{
	int n = find_arg(arg, argc, argv);

	if (n < 0) return NULL;
	++n;
	if (n >= argc)
	{
		fprintf(stderr, "Syntax error: use '%s <type>'\n", arg);
		exit(1);
	}
	if (!strcmpi(argv[n], "auto")) return NULL;
	return argv[n];
}

