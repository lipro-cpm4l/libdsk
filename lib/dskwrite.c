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

#include "drvi.h"

dsk_err_t dsk_pwrite(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
                              const void *buf, dsk_pcyl_t cylinder,
                              dsk_phead_t head, dsk_psect_t sector)
{
	DRV_CLASS *dc;
	if (!self || !geom || !buf || !self->dr_class) return DSK_ERR_BADPTR;

	dc = self->dr_class;

	if (!dc->dc_write) return DSK_ERR_NOTIMPL;
	return (dc->dc_write)(self,geom,buf,cylinder,head,sector);	
}


dsk_err_t dsk_lwrite(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
                              const void *buf, dsk_lsect_t sector)
{
        dsk_pcyl_t  c;
        dsk_phead_t h;
        dsk_psect_t s;
        dsk_err_t e;
 
        e = dg_ls2ps(geom, sector, &c, &h, &s);
        if (e != DSK_ERR_OK) return e;
        return dsk_pwrite(self, geom, buf, c, h, s);
}


dsk_err_t dsk_xwrite(DSK_DRIVER *self, const DSK_GEOMETRY *geom, 
			const void *buf, 
                        dsk_pcyl_t cylinder,   dsk_phead_t head,
                        dsk_pcyl_t cyl_expect, dsk_phead_t head_expect,
                        dsk_psect_t sector, size_t sector_len)
{
        DRV_CLASS *dc;
        if (!self || !geom || !buf || !self->dr_class) return DSK_ERR_BADPTR;

        dc = self->dr_class;

        if (!dc->dc_xwrite) return DSK_ERR_NOTIMPL;
        return (dc->dc_xwrite)(self,geom,buf,cylinder,head,
                                cyl_expect, head_expect, sector, sector_len);
}
                                                                                        
