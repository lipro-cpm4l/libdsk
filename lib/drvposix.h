/***************************************************************************
 *                                                                         *
 *    LIBDSK: General floppy and diskimage access library                  *
 *    Copyright (C) 2001, 2017  John Elliott <seasip.webmaster@gmail.com>  *
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

/* Declarations for the POSIX driver */

typedef struct
{
        DSK_DRIVER px_super;
        FILE *px_fp;
	int   px_readonly;
	unsigned long  px_filesize;
	dsk_sides_t px_sides;
	DSK_GEOMETRY *px_export_geom;
} POSIX_DSK_DRIVER;

dsk_err_t posix_openalt(DSK_DRIVER *self, const char *filename);
dsk_err_t posix_openoo(DSK_DRIVER *self, const char *filename);
dsk_err_t posix_openob(DSK_DRIVER *self, const char *filename);
dsk_err_t posix_creatalt(DSK_DRIVER *self, const char *filename);
dsk_err_t posix_creatoo(DSK_DRIVER *self, const char *filename);
dsk_err_t posix_creatob(DSK_DRIVER *self, const char *filename);
dsk_err_t posix_close(DSK_DRIVER *self);
dsk_err_t posix_read(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
                              void *buf, dsk_pcyl_t cylinder,
                              dsk_phead_t head, dsk_psect_t sector);
dsk_err_t posix_write(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
                              const void *buf, dsk_pcyl_t cylinder,
                              dsk_phead_t head, dsk_psect_t sector);
dsk_err_t posix_format(DSK_DRIVER *self, DSK_GEOMETRY *geom,
                                dsk_pcyl_t cylinder, dsk_phead_t head,
                                const DSK_FORMAT *format, unsigned char filler);
dsk_err_t posix_xseek(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
                                dsk_pcyl_t cylinder, dsk_phead_t head);
dsk_err_t posix_status(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
                                dsk_phead_t head, unsigned char *result);
dsk_err_t posix_to_ldbs(DSK_DRIVER *self, struct ldbs **result, DSK_GEOMETRY *geom);
dsk_err_t posix_from_ldbs(DSK_DRIVER *self, struct ldbs *source, DSK_GEOMETRY *geom);

