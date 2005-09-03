/***************************************************************************
 *                                                                         *
 *    LIBDSK: General floppy and diskimage access library                  *
 *    Copyright (C) 2001,2005  John Elliott <jce@seasip.demon.co.uk>       *
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

#if defined(HAVE_WINDOWS_H) && defined (_WIN32)

typedef struct w32serial_remote_data
{
	REMOTE_DATA super;
	HANDLE hComm;
	DCB dcb;
	char *filename;
	int baud;
	int crtscts;
	/* We won't allow changes in framing. LibDsk needs an 8-bit
	 * channel so force 8, N, 1 */
} W32SERIAL_REMOTE_DATA;

extern REMOTE_CLASS rpc_w32serial;

dsk_err_t w32serial_open(DSK_PDRIVER pDriver, const char *name, char *nameout);
dsk_err_t w32serial_close(DSK_PDRIVER pDriver);
dsk_err_t w32serial_call(DSK_PDRIVER pDriver, unsigned char *input, 
		int inp_len, unsigned char *output, int *out_len);

#endif /* def HAVE_TERMIOS_H */
