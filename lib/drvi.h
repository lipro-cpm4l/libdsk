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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "config.h"
#include "libdsk.h"
#include "drv.h"

#ifndef DISABLE_FLOPPY
# ifdef HAVE_LINUX_FD_H
#  include "linux/fd.h"
#  ifdef HAVE_LINUX_FDREG_H
#   define LINUXFLOPPY 
#   include "linux/fdreg.h"
#  endif
# endif

# ifdef HAVE_WINDOWS_H 
#  include <windows.h>
# endif

#ifdef HAVE_WINIOCTL_H
#  define WIN32FLOPPY 
#  include <winioctl.h>
# endif
#endif

/* See if we have any floppy drivers that take parameters of the form A: */
#ifdef WIN32FLOPPY 
# define ANYFLOPPY
#endif
#ifdef WIN16FLOPPY 
# define ANYFLOPPY
#endif
#ifdef DOS32FLOPPY 
# define ANYFLOPPY
#endif
#ifdef DOS16FLOPPY 
# define ANYFLOPPY
#endif

