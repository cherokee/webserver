/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *
 * Copyright (C) 2001-2009 Alvaro Lopez Ortega
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */ 

#include "common-internal.h"
#include "shm.h"
#include "util.h"

#ifdef HAVE_SYS_MMAN_H
# include <sys/mman.h>
#endif

#include <sys/stat.h>
#include <unistd.h>

ret_t
cherokee_shm_init (cherokee_shm_t *shm)
{
	shm->len = 0;
	shm->mem = NULL;
	
	cherokee_buffer_init (&shm->name);
	
	return ret_ok;
}


ret_t
cherokee_shm_mrproper (cherokee_shm_t *shm)
{
	if (shm->mem) {
		munmap (shm->mem, shm->len);
	}
	
	cherokee_buffer_mrproper (&shm->name);
	return ret_ok;
}


ret_t
cherokee_shm_create (cherokee_shm_t *shm, char *name, size_t len)
{
	int re;
	int fd;
	
	fd = shm_open (name, O_RDWR | O_EXCL | O_CREAT, 0600);
	if (fd < 0) {
		return ret_error;
	}

	re = ftruncate (fd, len);
	if (re < 0) {
		close (fd);
		return ret_error;
	}
	
	shm->mem = mmap (0, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (shm->mem == MAP_FAILED) {
		shm->mem = NULL;
		close (fd);
		return ret_error;
	}

	close (fd);

	shm->len = len;
	cherokee_buffer_add (&shm->name, name, strlen(name));

	return ret_ok;
}

ret_t
cherokee_shm_map (cherokee_shm_t    *shm,
		  cherokee_buffer_t *name)
{
	int         re;
	int         fd;
	struct stat info;

	fd = shm_open (name->buf, O_RDWR, 0600);
	if (fd < 0) {
		return ret_error;
	}

	re = fstat (fd, &info);
	if (re != 0) {
		return ret_error;
	}

	shm->mem = mmap (0, info.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (shm->mem == MAP_FAILED) {
		close (fd);
		shm->mem = NULL;
		return ret_error;
	}

	close (fd);

	cherokee_buffer_clean      (&shm->name);
	cherokee_buffer_add_buffer (&shm->name, name);

	return ret_ok;
}


ret_t
cherokee_sem_init (cherokee_sem_t    *sem,
		   cherokee_buffer_t *name)
{
	cherokee_buffer_init       (&sem->name);
	cherokee_buffer_add_buffer (&sem->name, name);

	sem->sem = sem_open (name->buf, 0, 0600);
	if (sem->sem == NULL) {
		return ret_error;
	}

	return ret_ok;
}


ret_t
cherokee_sem_mrproper (cherokee_sem_t *sem)
{
	if (sem->sem != NULL) {
		sem_close (sem->sem);
		sem->sem = NULL;
	}
		
	cherokee_buffer_mrproper (&sem->name);
	return ret_ok;
}


ret_t
cherokee_sem_post (cherokee_sem_t *sem)
{
	int re;

	if (unlikely (sem->sem == NULL))
		return ret_error;

	re = sem_post (sem->sem);
	return (re == 0) ? ret_ok : ret_error;
}
