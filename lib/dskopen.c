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
/* Functions to open an image file and select the driver */

#include "drvi.h"
#include "drivers.h"



static DRV_CLASS *classes[] = 
{

#include "drivers.inc"

	NULL
};

static void dr_construct(DSK_DRIVER *self, DRV_CLASS *dc)
{
	memset(self, 0, dc->dc_selfsize);
	self->dr_forcehead = -1;	
	self->dr_class = dc;

}


/* Attempt to create a DSK file with driver number <ndrv> */
static dsk_err_t dsk_icreat(DSK_DRIVER **self, const char *filename, int ndrv)
{
	DRV_CLASS *dc = classes[ndrv];
	dsk_err_t err;

	if (!dc) return DSK_ERR_BADPTR;
	
	(*self) = dsk_malloc(dc->dc_selfsize);
	if (!*self) return DSK_ERR_NOMEM;
	dr_construct(*self, dc);

	err = (dc->dc_creat)(*self, filename);
	if (err == DSK_ERR_OK) return err;
	dsk_free (*self);
	*self = NULL;
	return err;
}


/* Attempt to open a DSK file with driver <ndrv> */
static dsk_err_t dsk_iopen(DSK_DRIVER **self, const char *filename, int ndrv)
{
	DRV_CLASS *dc = classes[ndrv];
	dsk_err_t err;

	if (!dc) return DSK_ERR_BADPTR;
	
	(*self) = dsk_malloc(dc->dc_selfsize);
	if (!*self) return DSK_ERR_NOMEM;
	dr_construct(*self, dc);

	err = (dc->dc_open)(*self, filename);
	if (err == DSK_ERR_OK) return err;
	dsk_free (*self);
	*self = NULL;
	return err;
}

/* Create a disc file, of type "type" with name "filename". */
dsk_err_t dsk_creat(DSK_DRIVER **self, const char *filename, const char *type)
{
	int ndrv;

	if (!self || !filename || !type) return DSK_ERR_BADPTR;
	


	for (ndrv = 0; classes[ndrv]; ndrv++)
	{
		if (!strcmp(type, classes[ndrv]->dc_drvname))
			return dsk_icreat(self, filename, ndrv);
	}
	return DSK_ERR_NODRVR;
}



/* Close a DSK file. Frees the pointer and sets it to null. */
dsk_err_t dsk_open(DSK_DRIVER **self, const char *filename, const char *type)
{
	int ndrv;
	dsk_err_t e;

	if (!self || !filename) return DSK_ERR_BADPTR;
	
	if (type)
	{
		for (ndrv = 0; classes[ndrv]; ndrv++)
		{
			if (!strcmp(type, classes[ndrv]->dc_drvname))
				return dsk_iopen(self, filename, ndrv);
		}
		return DSK_ERR_NODRVR;
	}
	for (ndrv = 0; classes[ndrv]; ndrv++)
	{
		e = dsk_iopen(self, filename, ndrv);
		if (e != DSK_ERR_NOTME) return e;
	}	
	return DSK_ERR_NOTME;
}

/* Close a DSK file. Frees the pointer and sets it to null. */

dsk_err_t dsk_close(DSK_DRIVER **self)
{
	dsk_err_t e;

	if (!self || (!(*self)) || (!(*self)->dr_class))    return DSK_ERR_BADPTR;

	e = ((*self)->dr_class->dc_close)(*self);
	dsk_free (*self);
	*self = NULL;
	return e;
}

/* If "index" is in range, returns the n'th driver name in (*drvname).
 * Else sets (*drvname) to null. */
dsk_err_t dsk_type_enum(int index, char **drvname)
{
	int ndrv;

	if (!drvname) return DSK_ERR_BADPTR;

	for (ndrv = 0; classes[ndrv]; ndrv++)
	{
		if (index == ndrv)
		{
			*drvname = classes[ndrv]->dc_drvname;
			return DSK_ERR_OK;
		}
	}
	*drvname = NULL;
	return DSK_ERR_NODRVR;
}


unsigned char dsk_get_psh(size_t secsize)
{
	unsigned char psh;

	for (psh = 0; secsize > 128; secsize /= 2) psh++;
	return psh;
}


const char *dsk_drvname(DSK_DRIVER *self)
{
	if (!self || !self->dr_class || !self->dr_class->dc_drvname)
		return "(null)";
	return self->dr_class->dc_drvname;
}


const char *dsk_drvdesc(DSK_DRIVER *self)
{
	if (!self || !self->dr_class || !self->dr_class->dc_description)
		return "(null)";
	return self->dr_class->dc_description;
}


