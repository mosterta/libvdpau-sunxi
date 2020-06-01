/*
 * Copyright (c) 2013-2014 Jens Kuske <jenskuske@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include "ve.h"
#include <string.h>
#include <math.h>

#if defined(VALGRIND_DEBUG)
#include <valgrind/ammt_reqs.h>
#endif

#define DEVICE "/dev/cedar_dev"
#define PAGE_OFFSET (0xc0000000) // from kernel
#define PAGE_SIZE (4096)

enum IOCTL_CMD
{
	IOCTL_UNKOWN = 0x100,
	IOCTL_GET_ENV_INFO,
	IOCTL_WAIT_VE,
	IOCTL_RESET_VE,
	IOCTL_ENABLE_VE,
	IOCTL_DISABLE_VE,
	IOCTL_SET_VE_FREQ,

	IOCTL_WAIT_VE_DE_DISP2 = 0x102,
	IOCTL_WAIT_VE_EN_DISP2,
	IOCTL_RESET_VE_DISP2,
	IOCTL_ENABLE_VE_DISP2,
	IOCTL_DISABLE_VE_DISP2,
	IOCTL_SET_VE_FREQ_DISP2,

	IOCTL_CONFIG_AVS2 = 0x200,
	IOCTL_GETVALUE_AVS2 ,
	IOCTL_PAUSE_AVS2 ,
	IOCTL_START_AVS2 ,
	IOCTL_RESET_AVS2 ,
	IOCTL_ADJUST_AVS2,
	IOCTL_ENGINE_REQ,
	IOCTL_ENGINE_REL,
	IOCTL_ENGINE_CHECK_DELAY,
	IOCTL_GET_IC_VER,
	IOCTL_ADJUST_AVS2_ABS,
	IOCTL_FLUSH_CACHE
};

struct ve_info
{
	uint32_t reserved_mem;
	int reserved_mem_size;
	uint32_t registers;
};

struct cedarv_cache_range
{
	long start;
	long end;
};

struct memchunk_t
{
	uint32_t phys_addr;
	int size;
	void *virt_addr;
	struct memchunk_t *next;
};

static struct ve_dev
{
	int fd;
	void *regs;
	int version;
#if USE_UMP == 0
	struct memchunk_t first_memchunk;
	pthread_rwlock_t memory_lock;
#endif
	pthread_mutex_t device_lock;
    int initialized;
    unsigned int refCnt;
    int reservedEngine;
} ve = { .fd = -1, 
#if USE_UMP == 0
	.memory_lock = PTHREAD_RWLOCK_INITIALIZER, 
#endif
        .device_lock = PTHREAD_MUTEX_INITIALIZER,
        .initialized = 0,
        .refCnt = 0,
        .reservedEngine = -1,
};

int cedarv_allocateEngine(int engine)
{
  int status = 0;
  
  if (pthread_mutex_lock(&ve.device_lock))
    return 0;
  if(ve.reservedEngine == -1)
  {
    ve.reservedEngine = engine;
    status = 1;
  }
  pthread_mutex_unlock(&ve.device_lock);
  return status;
}

int cedarv_VeReset()
{
  if(ve.version < 0x1639) 
  {
    ioctl(ve.fd, IOCTL_ENABLE_VE, 0);
    ioctl(ve.fd, IOCTL_SET_VE_FREQ, 320);
    ioctl(ve.fd, IOCTL_RESET_VE, 0);
  }
  else
  {
    ioctl(ve.fd, IOCTL_ENABLE_VE_DISP2, 0);
    ioctl(ve.fd, IOCTL_SET_VE_FREQ_DISP2, 320);
    ioctl(ve.fd, IOCTL_RESET_VE_DISP2, 0);
  }
  writel(0x1, ve.regs + CEDARV_RESET);
  usleep(20000); //20ms
  writel(0x0, ve.regs + CEDARV_RESET);

  return 0;
}

int cedarv_freeEngine()
{
  int status = 1;
  
  if (pthread_mutex_lock(&ve.device_lock))
    return -1;
  ve.reservedEngine = -1;
  pthread_mutex_unlock(&ve.device_lock);
  return status;
}

int cedarv_open(void)
{
        if (pthread_mutex_lock(&ve.device_lock))
                return 0;
        if(ve.initialized == 0)
        {
             if (ve.fd != -1)
             {
		 printf("ve.fd != -1\n");
		 return 0;
             }

             struct ve_info info;

             ve.fd = open(DEVICE, O_RDWR);
	     if (ve.fd == -1)
             {
		 printf("could not open %s\n", DEVICE);
		 return 0;
	     }

	     if (ioctl(ve.fd, IOCTL_GET_ENV_INFO, (void *)(&info)) == -1)
	     {
		 printf("ioctl get_env_info failed!\n");
		 goto err;
	     }

	     ve.regs = mmap(NULL, 0x800, PROT_READ | PROT_WRITE, MAP_SHARED, ve.fd, info.registers);
	     if (ve.regs == MAP_FAILED)
	     {
		 printf("mmap failed!\n");
		 goto err;
	     }
#if USE_UMP == 0
	     ve.first_memchunk.phys_addr = info.reserved_mem - PAGE_OFFSET;
	     ve.first_memchunk.size = info.reserved_mem_size;
#endif

#if defined(VALGRIND_DEBUG)
             VALGRIND_PRINTF("regs base addreess=%p\n", ve.regs);

             AMMT_SET_REGS_BASE(ve.regs);
#endif

             ioctl(ve.fd, IOCTL_ENGINE_REQ, 0);

             ve.version = readl(ve.regs + CEDARV_VERSION) >> 16;

             cedarv_VeReset();
             writel(0x1, ve.regs + CEDARV_RESET);
             usleep(20000); //20ms
             writel(0x0, ve.regs + CEDARV_RESET);
         
	     writel(0x00130007, ve.regs + CEDARV_CTRL);

#if USE_UMP
	     if(ump_open() != UMP_OK)
	     {
                  printf("ump_open failed!\n");
	          goto err;
	     }
#endif
             ve.initialized = 1;
        }
        ve.refCnt ++;

        pthread_mutex_unlock(&ve.device_lock);
	return 1;

err:
	close(ve.fd);
	ve.fd = -1;
        pthread_mutex_unlock(&ve.device_lock);

	return 0;
}

void cedarv_close(void)
{
        if(ve.initialized && --ve.refCnt == 0)
        {
	    if (ve.fd == -1)
		return;

            if (ve.version < 1639)
               ioctl(ve.fd, IOCTL_DISABLE_VE, 0);
            else
               ioctl(ve.fd, IOCTL_DISABLE_VE_DISP2, 0);

	    ioctl(ve.fd, IOCTL_ENGINE_REL, 0);

	    munmap(ve.regs, 0x800);
	    ve.regs = NULL;

	    close(ve.fd);
	    ve.fd = -1;
#if USE_UMP
	    ump_close();
#endif
            ve.initialized = 0;
        }
}

int cedarv_get_version(void)
{
	return ve.version;
}

int cedarv_wait(int timeout)
{
	if (ve.fd == -1)
		return -1;

	if (ve.version < 1669)
		return ioctl(ve.fd, IOCTL_WAIT_VE, timeout);
	else
		return ioctl(ve.fd, IOCTL_WAIT_VE_DE_DISP2, timeout);
}

void *cedarv_get(int engine, uint32_t flags)
{
	if (pthread_mutex_lock(&ve.device_lock))
		return NULL;

	writel(0x00130000 | (engine & 0xf) | (flags & ~0xf), ve.regs + CEDARV_CTRL);

	return ve.regs;
}

void cedarv_put(void)
{
	writel(0x00130007, ve.regs + CEDARV_CTRL);
	pthread_mutex_unlock(&ve.device_lock);
}

void* cedarv_get_regs()
{
	return ve.regs;
}
#if USE_UMP

CEDARV_MEMORY cedarv_malloc(int size)
{
  CEDARV_MEMORY mem;
  mem.mem_id = ump_ref_drv_allocate (size, UMP_REF_DRV_CONSTRAINT_PHYSICALLY_LINEAR);
  if(mem.mem_id == UMP_INVALID_MEMORY_HANDLE)
  {
    printf("could not allocate ump buffer!\n");
    exit(1);
  }
  return mem;
}

int cedarv_isValid(CEDARV_MEMORY mem)
{
  return (mem.mem_id != UMP_INVALID_MEMORY_HANDLE);
}

void cedarv_free(CEDARV_MEMORY mem)
{
  ump_reference_release(mem.mem_id);
}

uint32_t cedarv_virt2phys(CEDARV_MEMORY mem)
{
  return (uint32_t)ump_phys_address_get(mem.mem_id);
}

void cedarv_flush_cache(CEDARV_MEMORY mem, int len)
{
  ump_cpu_msync_now(mem.mem_id, UMP_MSYNC_CLEAN_AND_INVALIDATE, 0, len);
}
void cedarv_memcpy(CEDARV_MEMORY dst, size_t offset, const void * src, size_t len)
{
  ump_write(dst.mem_id, offset, src, len);
}
void cedarv_memset(CEDARV_MEMORY dst, unsigned char value, size_t len)
{
  void* mem = ump_mapped_pointer_get(dst.mem_id);
  memset(mem, value, len);
}
void* cedarv_getPointer(CEDARV_MEMORY mem)
{
  return ump_mapped_pointer_get(mem.mem_id);
}

unsigned char cedarv_byteAccess(CEDARV_MEMORY mem, size_t offset)
{
  char *ptr = (char*)ump_mapped_pointer_get(mem.mem_id);
  return ptr[offset];
}

size_t cedarv_getSize(CEDARV_MEMORY mem)
{
  return ump_size_get(mem.mem_id);
}

void cedarv_setBufferInvalid(CEDARV_MEMORY mem)
{
  mem.mem_id = UMP_INVALID_MEMORY_HANDLE;
}

#else

void *cedarv_malloc(int size)
{
	if (ve.fd == -1)
		return NULL;

	if (pthread_rwlock_wrlock(&ve.memory_lock))
		return NULL;

	void *addr = NULL;

	size = (size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
	struct memchunk_t *c, *best_chunk = NULL;
	for (c = &ve.first_memchunk; c != NULL; c = c->next)
	{
		if(c->virt_addr == NULL && c->size >= size)
		{
			if (best_chunk == NULL || c->size < best_chunk->size)
				best_chunk = c;

			if (c->size == size)
				break;
		}
	}

	if (!best_chunk)
		goto out;

	int left_size = best_chunk->size - size;

	addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, ve.fd, best_chunk->phys_addr + PAGE_OFFSET);
	if (addr == MAP_FAILED)
	{
		addr = NULL;
		goto out;
	}

	best_chunk->virt_addr = addr;
	best_chunk->size = size;

	if (left_size > 0)
	{
		c = malloc(sizeof(struct memchunk_t));
		c->phys_addr = best_chunk->phys_addr + size;
		c->size = left_size;
		c->virt_addr = NULL;
		c->next = best_chunk->next;
		best_chunk->next = c;
	}

out:
	pthread_rwlock_unlock(&ve.memory_lock);
	return addr;
}

int cedarv_isValid(void* mem)
{
  return mem != NULL;
}
void cedarv_free(void *ptr)
{
	if (ve.fd == -1)
		return;

	if (ptr == NULL)
		return;

	if (pthread_rwlock_wrlock(&ve.memory_lock))
		return;

	struct memchunk_t *c;
	for (c = &ve.first_memchunk; c != NULL; c = c->next)
	{
		if (c->virt_addr == ptr)
		{
			munmap(ptr, c->size);
			c->virt_addr = NULL;
			break;
		}
	}

	for (c = &ve.first_memchunk; c != NULL; c = c->next)
	{
		if (c->virt_addr == NULL)
		{
			while (c->next != NULL && c->next->virt_addr == NULL)
			{
				struct memchunk_t *n = c->next;
				c->size += n->size;
				c->next = n->next;
				free(n);
			}
		}
	}

	pthread_rwlock_unlock(&ve.memory_lock);
}

uint32_t cedarv_virt2phys(void *ptr)
{
	if (ve.fd == -1)
		return 0;

	if (pthread_rwlock_rdlock(&ve.memory_lock))
		return 0;

	uint32_t addr = 0;

	struct memchunk_t *c;
	for (c = &ve.first_memchunk; c != NULL; c = c->next)
	{
		if (c->virt_addr == NULL)
			continue;

		if (c->virt_addr == ptr)
		{
			addr = c->phys_addr;
			break;
		}
		else if (ptr > c->virt_addr && ptr < (c->virt_addr + c->size))
		{
			addr = c->phys_addr + (ptr - c->virt_addr);
			break;
		}
	}

	pthread_rwlock_unlock(&ve.memory_lock);
	return addr;
}

void cedarv_flush_cache(void *start, int len)
{
	if (ve.fd == -1)
		return;

	struct cedarv_cache_range range =
	{
		.start = (int)start,
		.end = (int)(start + len)
	};

	ioctl(ve.fd, IOCTL_FLUSH_CACHE, (void*)(&range));
}

void cedarv_memcpy(void* dst, size_t offset, const void * src, size_t len)
{
	memcpy((char*)dst + offset, src, len);
}

void* cedarv_getPointer(CEDARV_MEMORY mem)
{
  return mem;
}

unsigned char cedarv_byteAccess(void* mem, size_t offset)
{
  char *ptr = (char*)mem;
  return ptr[offset];
}

void cedarv_setBufferInvalid(void* mem)
{
  mem = NULL;
}

#endif
