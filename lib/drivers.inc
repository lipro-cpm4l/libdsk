/***************************************************************************
 *                                                                         *
 *    LIBDSK: General floppy and diskimage access library                  *
 *    Copyright (C) 2001  John Elliott <jce@seasip.demon.co.uk>            *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
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

/* Note: Order does matter in this file.
 * This is the order in which drivers will be tested when format is being
 * autodetected. The order is:
 *
 * 1. Devices whose names aren't the names of files - ie, the Windows floppy
 */

#ifdef WIN32FLOPPY
	&dc_win32,
#endif
/* 2. Disc images which can be detected by magic numbers or other means. This 
 *    includes CPCEMU discs and Linux floppy drives. Linux floppy comes first
 *    because it makes detection of CPC formats much faster. */
#ifdef LINUXFLOPPY
	&dc_linux,
#endif
	&dc_cpcemu,	/* Normal .DSK */
	&dc_dqk,	/* Compressed .DSK */

/* 3. Raw files with no magic number. */
	&dc_posix,
/* 4. Raw files that will never be selected automatically. For example, 
 *   the MYZ80 drive image cannot be reliably identified, so it must be 
 *   explicitly specified. */
	&dc_myz80,
	&dc_cfi,
