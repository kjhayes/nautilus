/*
 * This file is part of the Nautilus AeroKernel developed
 * by the Hobbes and V3VEE Projects with funding from the
 * United States National  Science Foundation and the Department of Energy.
 *
 * The V3VEE Project is a joint project between Northwestern University
 * and the University of New Mexico.  The Hobbes Project is a collaboration
 * led by Sandia National Laboratories that includes several national
 * laboratories and universities. You can find out more at:
 * http://www.v3vee.org  and
 * http://xstack.sandia.gov/hobbes
 *
 * Copyright (c) 2016, Peter Dinda
 * Copyright (c) 2016, Yang Wu, Fei Luo and Yuanhui Yang
 * Copyright (c) 2017, Kyle C. Hale
 * Copyright (c) 2016, The V3VEE Project  <http://www.v3vee.org>
 *                     The Hobbes Project <http://xstack.sandia.gov/hobbes>
 * All rights reserved.
 *
 * Authors:  Peter Dinda <pdinda@northwestern.edu>
 *           Yang Wu, Fei Luo and Yuanhui Yang
 *           {YangWu2015, FeiLuo2015, YuanhuiYang2015}@u.northwestern.edu
 *           Kyle C. Hale <khale@cs.iit.edu>
 *
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "LICENSE.txt".
 */

#include <nautilus/nautilus.h>
#include <nautilus/init.h>
#include <nautilus/thread.h>
#include <nautilus/timer.h>
#include <nautilus/waitqueue.h>
#include <nautilus/list.h>
#include <dev/ps2.h>
#include <nautilus/vc.h>
#include <nautilus/chardev.h>
#include <nautilus/gpudev.h>
#include <nautilus/printk.h>
#include <dev/serial.h>
#include <dev/vga_early.h>

#include <nautilus/shell.h>

#ifdef NAUT_CONFIG_XEON_PHI
#include <arch/k1om/xeon_phi.h>
#endif
#ifdef NAUT_CONFIG_HVM_HRT
#include <arch/hrt/hrt.h>
#endif

#define DEFAULT_VC_WIDTH VGA_WIDTH
#define DEFAULT_VC_HEIGHT VGA_HEIGHT

#define CHARDEV_CONSOLE_STACK_SIZE PAGE_SIZE_2MB
#define VC_LIST_STACK_SIZE PAGE_SIZE_2MB

#ifdef NAUT_CONFIG_ARCH_X86
#define INTERRUPT __attribute__((target("no-sse")))
#else
#define INTERRUPT
#endif

#ifndef NAUT_CONFIG_DEBUG_VIRTUAL_CONSOLE
#undef DEBUG_PRINT
#define DEBUG_PRINT(fmt, args...)
#endif

#define ERROR(fmt, args...) ERROR_PRINT("vc: " fmt, ##args)
#define DEBUG(fmt, args...) DEBUG_PRINT("vc: " fmt, ##args)
#define INFO(fmt, args...) INFO_PRINT("vc: " fmt, ##args)

static int up=0;

static spinlock_t state_lock;

#define STATE_LOCK_CONF uint8_t _state_lock_flags
#define STATE_LOCK() _state_lock_flags = spin_lock_irq_save(&state_lock)
#define STATE_UNLOCK() spin_unlock_irq_restore(&state_lock, _state_lock_flags);

#define BUF_LOCK_CONF uint8_t _buf_lock_flags
#define BUF_LOCK(cons) _buf_lock_flags = spin_lock_irq_save(&cons->buf_lock)
#define BUF_UNLOCK(cons) spin_unlock_irq_restore(&cons->buf_lock, _buf_lock_flags);

#define QUEUE_LOCK_CONF uint8_t _queue_lock_flags
#define QUEUE_LOCK(cons) _queue_lock_flags = spin_lock_irq_save(&cons->queue_lock)
#define QUEUE_UNLOCK(cons) spin_unlock_irq_restore(&cons->queue_lock, _queue_lock_flags);

// List of all VC in the system
static struct list_head vc_list;

// Current VC for native console
static struct nk_virtual_console *cur_vc = NULL;

// Typically used VCs
static struct nk_virtual_console *default_vc = NULL;
static struct nk_virtual_console *log_vc = NULL;
static struct nk_virtual_console *list_vc = NULL;

// We can have any number of chardev consoles
// each of which has a current vc
struct chardev_console {
    int    inited;       // handler thread is up
    int    shutdown;     // do shutdown (1 => start; trips init back to 0 when done)
    nk_thread_id_t   tid;  // thread handling this console
    char   name[DEV_NAME_LEN]; // device name
    struct nk_char_dev *dev; // device for I/O
    struct nk_virtual_console *cur_vc; // current virtual console
    struct list_head chardev_node;  // for list of chardev consoles
};

struct gpudev_console 
{
    char name[DEV_NAME_LEN];
    struct nk_gpu_dev *dev;

    nk_gpu_dev_video_mode_t mode;
    nk_gpu_dev_box_t box;

    struct nk_virtual_console *private_vc;
    struct nk_virtual_console **cur_vc;

    struct list_head gpudev_node;
};

// List of all chardev consoles in the system
static struct list_head chardev_console_list;
// List of all gpudev consoles in the system
static struct list_head gpudev_console_list;

// Broadcast updates to chardev consoles
static void chardev_consoles_putchar(struct nk_virtual_console *vc, char data);
static void chardev_consoles_print(struct nk_virtual_console *vc, char *data);

// Broadcast updates to gpudev consoles
static void gpudev_consoles_set_cursor(struct nk_virtual_console *vc, uint32_t x, uint32_t y, int state_lock_held);
static void gpudev_consoles_set_char(struct nk_virtual_console *vc, uint32_t x, uint32_t y, char symbol, uint8_t attr);
static void gpudev_consoles_display_buffer(struct nk_virtual_console *vc, int state_lock_held);
static void gpudev_consoles_flush(struct nk_virtual_console *vc, int state_lock_held);

static nk_thread_id_t list_tid;

#define Keycode_QUEUE_SIZE 256
#define Scancode_QUEUE_SIZE 512

struct nk_virtual_console {
  enum nk_vc_type type;
  char name[32];
  // held with interrupts off => coordinate with interrupt handler
  spinlock_t       queue_lock;
  // held without interrupt change => coordinate with threads
  spinlock_t       buf_lock;

  union queue{
    nk_scancode_t s_queue[Scancode_QUEUE_SIZE];
    nk_keycode_t k_queue[Keycode_QUEUE_SIZE];
  } keyboard_queue;

  nk_gpu_dev_charmap_t *buffer;
  nk_gpu_dev_coordinate_t cursor;

  uint8_t cur_attr, fill_attr;
  uint16_t head, tail;
  struct nk_vc_ops *ops;
  void *ops_priv;
  uint32_t num_threads;
  struct list_head vc_node;
  nk_wait_queue_t *waiting_threads;
  nk_timer_t *timer;
};


inline int nk_vc_is_active()
{
  return cur_vc!=0;
}

struct nk_virtual_console * nk_get_cur_vc()
{
    return cur_vc;
}

#ifdef NAUT_CONFIG_VIRTUAL_CONSOLE_DISPLAY_NAME

#define VC_TIMER_MS 500UL
#define VC_TIMER_NS (1000000UL*VC_TIMER_MS)

static void vc_timer_callback(void *state)
{
    struct nk_virtual_console *vc = (struct nk_virtual_console *)state;

    nk_vc_display_str_specific(vc,vc->name,strlen(vc->name),0xf0,vc->buffer->width-strlen(vc->name),0);

    nk_timer_reset(vc->timer, VC_TIMER_NS);
    nk_timer_start(vc->timer);
}

#endif


struct nk_virtual_console *nk_create_vc (char *name,
					 enum nk_vc_type new_vc_type,
					 uint8_t attr,
					 struct nk_vc_ops *ops,
					 void *priv_data)
{
  DEBUG("creating vc: \"%s\"\n", name);
  STATE_LOCK_CONF;
  int i;
  char buf[NK_WAIT_QUEUE_NAME_LEN];

  struct nk_virtual_console *new_vc = malloc(sizeof(struct nk_virtual_console));

  if (!new_vc) {
    ERROR("Failed to allocate new console\n");
    return NULL;
  }
  memset(new_vc, 0, sizeof(struct nk_virtual_console));

  new_vc->type = new_vc_type;

  DEBUG("setting new vc name...\n");
  strncpy(new_vc->name,name,32); new_vc->name[31]=0;
  new_vc->ops = ops;
  new_vc->cur_attr = attr;
  new_vc->fill_attr = attr;

  DEBUG("creating new vc charmap...\n");
  new_vc->buffer = nk_gpu_dev_charmap_create(DEFAULT_VC_WIDTH, DEFAULT_VC_HEIGHT);
  
  DEBUG("intiailizing locks...\n");
  spinlock_init(&new_vc->queue_lock);
  spinlock_init(&new_vc->buf_lock);
  new_vc->head = 0;
  new_vc->tail = 0;
  new_vc->num_threads = 0;
  snprintf(buf,NK_WAIT_QUEUE_NAME_LEN,"vc-%s-wait",name);
  DEBUG("creating waitqueue...\n");
  new_vc->waiting_threads = nk_wait_queue_create(buf);

#ifdef NAUT_CONFIG_VIRTUAL_CONSOLE_DISPLAY_NAME
  DEBUG("creating vc timer...\n");
  snprintf(buf,NK_TIMER_NAME_LEN,"vc-%s-timer",name);
  new_vc->timer = nk_timer_create(buf);
#endif

  DEBUG("clearing new vc...\n");
  // clear to new attr
  for (i = 0; i < new_vc->buffer->width*new_vc->buffer->height; i++) {
    new_vc->buffer->chars[i].symbol = ' ';
    new_vc->buffer->chars[i].attribute = new_vc->cur_attr;
  }

#ifdef NAUT_CONFIG_VIRTUAL_CONSOLE_DISPLAY_NAME
  // start timer
  DEBUG("setting vc timer...\n");
  if(nk_timer_set(new_vc->timer,VC_TIMER_NS,NK_TIMER_CALLBACK,vc_timer_callback,new_vc,0)) {
    ERROR("Failed to set timer for vc: \"%s\"\n", name);
    return NULL;
  }
  if(nk_timer_start(new_vc->timer)) {
    ERROR("Failed to start timer for vc: \"%s\"\n", name);
    return NULL;
  } 
#endif

  DEBUG("adding vc to list...\n");
  STATE_LOCK();
  list_add_tail(&new_vc->vc_node, &vc_list);
  STATE_UNLOCK();

  DEBUG("created vc: \"%s\"\n", name);

  return new_vc;
}


static int _switch_to_vc(struct nk_virtual_console *vc)
{
  if (!vc) {
    return 0;
  }
  if (vc!=cur_vc) {
    // This function doesn't make sense with the "gpudev" consoles system
    // We just need to make sure that we update a vc's charmap first, and never
    // only update the gpudev consoles without updating the charmap.
    //copy_display_to_vc(cur_vc);

    cur_vc = vc;
    gpudev_consoles_display_buffer(cur_vc, 1);
    gpudev_consoles_set_cursor(cur_vc, cur_vc->cursor.x, cur_vc->cursor.y, 1);
    gpudev_consoles_flush(cur_vc, 1);

#if NAUT_CONFIG_XEON_PHI
    phi_cons_set_cursor(cur_vc->cursor.x, cur_vc->cursor.y);
#endif
  }
  return 0;
}

// destroy the vc, acquiring/releasing  if needed
// if the lock is already held, release it and restore irq state
static int _destroy_vc(struct nk_virtual_console *vc, int havelock, uint8_t flags)
{
  STATE_LOCK_CONF;

  if (vc->num_threads || !nk_wait_queue_empty(vc->waiting_threads)) {
    ERROR("Cannot destroy virtual console that has threads\n");
    return -1;
  }

  if (vc==cur_vc) {
    _switch_to_vc(list_vc);
  }

  if (!havelock) {
    STATE_LOCK();
  } else {
    _state_lock_flags = flags;
  }
  list_del(&vc->vc_node);
  STATE_UNLOCK();

  // release lock early so the following can do output
  nk_wait_queue_destroy(vc->waiting_threads);
#ifdef NAUT_CONFIG_VIRTUAL_CONSOLE_DISPLAY_NAME
  nk_timer_cancel(vc->timer);
  nk_timer_destroy(vc->timer);
#endif
  free(vc);
  return 0;
}

int nk_destroy_vc(struct nk_virtual_console *vc)
{
  return _destroy_vc(vc,0,0);
}


int nk_bind_vc(nk_thread_t *thread, struct nk_virtual_console * cons)
{
  STATE_LOCK_CONF;
  if (!cons) {
    return -1;
  }
  STATE_LOCK();
  thread->vc = cons;
  cons->num_threads++;
  STATE_UNLOCK();
  return 0;
}

//release will destroy vc once the number of binders drops to zero
//except that the default vc is never destroyed
int nk_release_vc(nk_thread_t *thread)
{
  STATE_LOCK_CONF;

  if (!thread || !thread->vc) {
    return 0;
  }

  struct nk_virtual_console *vc = thread->vc;

  STATE_LOCK();

  thread->vc->num_threads--;
  if (thread->vc->num_threads == 0) {
    if (thread->vc != default_vc) {//don't destroy default_vc ever
       thread->vc = NULL;
       _destroy_vc(vc,1,_state_lock_flags);
       // lock is also now released
       return 0;
    }
  }
  thread->vc = NULL;
  STATE_UNLOCK();
  return 0;
}

int nk_switch_to_vc(struct nk_virtual_console *vc)
{
  int rc;
  STATE_LOCK_CONF;

  STATE_LOCK();
  rc = _switch_to_vc(vc);
  STATE_UNLOCK();

  return rc;
}

int nk_switch_to_prev_vc()
{
  struct list_head *cur_node = &cur_vc->vc_node;
  struct nk_virtual_console *target;

  if(cur_node->prev == &vc_list) {
    return 0;
  }
  target = container_of(cur_node->prev, struct nk_virtual_console, vc_node);
  return nk_switch_to_vc(target);
}

int nk_switch_to_next_vc()
{
  struct list_head *cur_node = &cur_vc->vc_node;
  struct nk_virtual_console *target;

  if(cur_node->next == &vc_list) {
    return 0;
  }

  target = container_of(cur_node->next, struct nk_virtual_console, vc_node);
  return nk_switch_to_vc(target);

}

INTERRUPT static int _vc_scrollup_specific(struct nk_virtual_console *vc)
{
  int i;
  uint32_t width = vc->buffer->width;
  uint32_t height = vc->buffer->height;

#ifdef NAUT_CONFIG_XEON_PHI
  if (vc==cur_vc) {
    phi_cons_notify_scrollup();
  }
#endif

  // Move all rows up by one
  for (i=0;
       i< width*(height-1);
       i++) {
    vc->buffer->chars[i] = vc->buffer->chars[i+width];
  }

  // Blank the bottom row
  for (i = 0;
       i < width;
       i++) {
    nk_gpu_dev_char_t *chr = &NK_GPU_DEV_CHARMAP_CHAR(vc->buffer, i, height-1);
    chr->symbol = ' ';
    chr->attribute = vc->fill_attr;
  }

  gpudev_consoles_display_buffer(vc, 0);
  gpudev_consoles_flush(vc, 0);

#ifdef NAUT_CONFIG_XEON_PHI
  if(vc == cur_vc) {
    phi_cons_notify_redraw();
  }
#endif

  return 0;
}

int nk_vc_scrollup_specific(struct nk_virtual_console *vc)
{
  BUF_LOCK_CONF;
  int rc;
  if (!vc) {
    return 0;
  }
  BUF_LOCK(vc);
  rc = _vc_scrollup_specific(vc);
  BUF_UNLOCK(vc);
  return rc;
}

int nk_vc_scrollup()
{
  int rc=0;
  struct nk_virtual_console *vc = get_cur_thread()->vc;

  if (!vc) {
    vc = default_vc;
  }
  if (vc) {
    BUF_LOCK_CONF;
    BUF_LOCK(vc);
    rc = _vc_scrollup_specific(vc);
    BUF_UNLOCK(vc);
  }

  return rc;
}

static int _vc_display_char_specific(struct nk_virtual_console *vc, uint8_t c, uint8_t attr, uint16_t x, uint16_t y)
{
  if (!vc) {
    return 0;
  }

  uint32_t width = vc->buffer->width;
  uint32_t height = vc->buffer->height;

  if(x >= width || y >= height) {
    return -1;
  } else {
    NK_GPU_DEV_CHARMAP_CHAR(vc->buffer,x,y).symbol = c;
    NK_GPU_DEV_CHARMAP_CHAR(vc->buffer,x,y).attribute = attr;

    gpudev_consoles_set_char(vc, x, y, c, attr);
    gpudev_consoles_set_cursor(vc, vc->cursor.x, vc->cursor.y, 0);
    gpudev_consoles_flush(cur_vc, 0);

#ifdef NAUT_CONFIG_XEON_PHI
    if(vc == cur_vc) {
      phi_cons_set_cursor(cur_vc->cursor.x, cur_vc->cursor.y);
    }
#endif
  }
  return 0;
}

static int _vc_display_char(uint8_t c, uint8_t attr, uint8_t x, uint8_t y)
{
  struct nk_virtual_console *thread_vc = get_cur_thread()->vc;
  if (!thread_vc) {
    thread_vc = default_vc;
  }
  if (!thread_vc) {
    return 0;
  } else {
    return _vc_display_char_specific(thread_vc,c,attr,x,y);
  }
}

int nk_vc_display_char_specific(struct nk_virtual_console *vc,
				uint8_t c, uint8_t attr, uint8_t x, uint8_t y)
{
  int rc = -1;

  if (vc) {
    BUF_LOCK_CONF;
    BUF_LOCK(vc);
    rc = _vc_display_char_specific(vc, c, attr, x, y);
    BUF_UNLOCK(vc);
  }
  return rc;
}

int nk_vc_display_char(uint8_t c, uint8_t attr, uint8_t x, uint8_t y)
{
  int rc=0;
  struct nk_virtual_console *vc = get_cur_thread()->vc;

  if (!vc) {
    vc = default_vc;
  }
  if (vc) {
    BUF_LOCK_CONF;
    BUF_LOCK(vc);
    rc = _vc_display_char(c,attr,x,y);
    BUF_UNLOCK(vc);
  }

  return rc;
}


int nk_vc_display_str_specific(struct nk_virtual_console *vc,
			       uint8_t *c, uint8_t n, uint8_t attr, uint8_t x, uint8_t y)
{
  int rc = 0;
  uint16_t i;
  uint16_t limit = (x+(uint16_t)n) > vc->buffer->width ? vc->buffer->width : x+(uint16_t)n;

  if (vc) {
    BUF_LOCK_CONF;
    BUF_LOCK(vc);
    for (i=0;i<limit;i++) {
	rc |= _vc_display_char_specific(vc, c[i], attr, x+i, y);
    }
    BUF_UNLOCK(vc);
  }
  return rc;
}

int nk_vc_display_str(uint8_t *c, uint8_t n, uint8_t attr, uint8_t x, uint8_t y)
{
  int rc=0;
  uint16_t i;
  struct nk_virtual_console *vc = get_cur_thread()->vc;
  uint16_t limit = (x+(uint16_t)n) > vc->buffer->width ? vc->buffer->width : x+(uint16_t)n;

  if (!vc) {
    vc = default_vc;
  }
  if (vc) {
    BUF_LOCK_CONF;
    BUF_LOCK(vc);
    for (i=0;i<limit;i++) {
	rc |= _vc_display_char_specific(vc, c[i], attr, x+i, y);
    }
    BUF_UNLOCK(vc);
  }

  return rc;
}



static int _vc_setpos(uint8_t x, uint8_t y)
{
  int rc=0;
  struct nk_virtual_console *vc = get_cur_thread()->vc;

  if (!vc) {
    vc = default_vc;
  }
  if (vc) {
    vc->cursor.x = x;
    vc->cursor.y = y;
    gpudev_consoles_set_cursor(vc, vc->cursor.x, vc->cursor.y, 0);
    gpudev_consoles_flush(vc, 0);
  }
  return 0;
}

static void _vc_getpos(uint8_t * x, uint8_t * y)
{
    struct nk_virtual_console *vc = get_cur_thread()->vc;

    if (!vc) {
        vc = default_vc;
    }

    if (vc) {
        if (x) *x = vc->cursor.x;
        if (y) *y = vc->cursor.y;
    }
}


int nk_vc_setpos(uint8_t x, uint8_t y)
{
  int rc=0;
  struct nk_virtual_console *vc = get_cur_thread()->vc;

  if (!vc) {
    vc = default_vc;
  }
  if (vc) {
    BUF_LOCK_CONF;
    BUF_LOCK(vc);
    rc = _vc_setpos(x,y);
    BUF_UNLOCK(vc)
  }

  return rc;
}


void nk_vc_getpos(uint8_t * x, uint8_t * y)
{
  struct nk_virtual_console *vc = get_cur_thread()->vc;

  if (!vc) {
    vc = default_vc;
  }
  if (vc) {
    BUF_LOCK_CONF;
    BUF_LOCK(vc);
    _vc_getpos(x,y);
    BUF_UNLOCK(vc)
  }
}


static int _vc_setpos_specific(struct nk_virtual_console *vc, uint8_t x, uint8_t y)
{
  int rc=0;

  if (vc) {
    vc->cursor.x = x;
    vc->cursor.y = y;
    gpudev_consoles_set_cursor(vc, vc->cursor.x, vc->cursor.y, 0);
    gpudev_consoles_flush(vc, 0);
  }

  return rc;
}


int nk_vc_setpos_specific(struct nk_virtual_console *vc, uint8_t x, uint8_t y)
{
  int rc=0;

  if (vc) {
    BUF_LOCK_CONF;
    BUF_LOCK(vc);
    rc = _vc_setpos_specific(vc,x,y);
    BUF_UNLOCK(vc);
  }

  return rc;
}


// display scrolling or explicitly on screen at a given location
INTERRUPT static int _vc_putchar_specific(struct nk_virtual_console *vc, uint8_t c) 
{
  if (!vc) {
    return 0;
  }

  if (c == ASCII_BS) {
	  vc->cursor.x--;
	  _vc_display_char_specific(vc, ' ', vc->fill_attr, vc->cursor.x, vc->cursor.y);
	  return 0;
  }

  if (c == '\n') {
    vc->cursor.x = 0;
#ifdef NAUT_CONFIG_XEON_PHI
    if (vc==cur_vc) {
      phi_cons_notify_line_draw(vc->cursor.y);
    }
#endif

    vc->cursor.y++;
    if (vc->cursor.y == vc->buffer->height) {
      _vc_scrollup_specific(vc);
      vc->cursor.y--;
    }

    gpudev_consoles_set_cursor(vc, vc->cursor.x, vc->cursor.y, 0);
    gpudev_consoles_flush(vc, 0);

#if NAUT_CONFIG_XEON_PHI
    if (vc==cur_vc) {
      phi_cons_set_cursor(vc->cursor.x,vc->cursor.y);
    }
#endif
    return 0;
  }


  _vc_display_char_specific(vc, c, vc->cur_attr, vc->cursor.x, vc->cursor.y);
  vc->cursor.x++;
  if (vc->cursor.x == vc->buffer->width) {
    vc->cursor.x = 0;
#ifdef NAUT_CONFIG_XEON_PHI
    if (vc==cur_vc) {
      phi_cons_notify_line_draw(vc->cursor.y);
    }
#endif
    vc->cursor.y++;
    if (vc->cursor.y == vc->buffer->height) {
      _vc_scrollup_specific(vc);
      vc->cursor.y--;
    }
  }

  gpudev_consoles_set_cursor(vc, vc->cursor.x, vc->cursor.y, 0);
  gpudev_consoles_flush(vc, 0);

#if NAUT_CONFIG_XEON_PHI
  if (vc==cur_vc) {
      phi_cons_set_cursor(vc->cursor.x,vc->cursor.y);
  }
#endif
  return 0;
}

// display scrolling or explicitly on screen at a given location
INTERRUPT static int _vc_putchar(uint8_t c) 
{
  struct nk_virtual_console *vc;
  struct nk_thread *t = get_cur_thread();

  if (!t || !(vc = t->vc)) {
    vc = default_vc;
  }

  if (!vc) {
    return 0;
  } else {
    return _vc_putchar_specific(vc,c);
  }
}

int nk_vc_putchar(uint8_t c)
{
  if (nk_vc_is_active()) {
    struct nk_virtual_console *vc;
    struct nk_thread *t = get_cur_thread();

    if (!t || !(vc = t->vc)) {
      vc = default_vc;
    }
    if (vc) {
      BUF_LOCK_CONF;
      BUF_LOCK(vc);
      _vc_putchar_specific(vc,c);
      BUF_UNLOCK(vc);
      chardev_consoles_putchar(vc, c);
    }
  } else {
    // VGA, phi_cons, hrt, and early UART's
    nk_pre_vc_putchar(c);
  }

#ifdef NAUT_CONFIG_VIRTUAL_CONSOLE_SERIAL_MIRROR_ALL
  serial_putchar(c);
#endif

  return c;
}

INTERRUPT static int _vc_print_specific(struct nk_virtual_console *vc, char *s) 
{
  if (!vc) {
    return 0;
  }
  while(*s) {
    _vc_putchar_specific(vc, *s);
    s++;
  }
  return 0;
}


static int vc_print_specific(struct nk_virtual_console *vc, char *s)
{
    if (!vc) {
        return 0;
    }
    BUF_LOCK_CONF;
    BUF_LOCK(vc);
    _vc_print_specific(vc,s);
    chardev_consoles_print(vc,s);
    BUF_UNLOCK(vc);
#ifdef NAUT_CONFIG_VIRTUAL_CONSOLE_SERIAL_MIRROR_ALL
    serial_write(s);
#endif
    return 0;
}

int nk_vc_print(char *s)
{
  if (nk_vc_is_active()) {
    struct nk_virtual_console *vc;
    struct nk_thread * t = get_cur_thread();

    if (!t || !(vc = t->vc)) {
      vc = default_vc;
    }
    if (vc) {
      BUF_LOCK_CONF;
      BUF_LOCK(vc);
      _vc_print_specific(vc,s);
      chardev_consoles_print(vc, s);
      BUF_UNLOCK(vc);
    }
  } else {
    // VGA, phi_cons, hrt, and early UART's
    nk_pre_vc_puts(s);
  }
#ifdef NAUT_CONFIG_VIRTUAL_CONSOLE_SERIAL_MIRROR_ALL
  serial_write(s);
#endif

  return 0;
}

int nk_vc_puts(char *s)
{
  nk_vc_print(s);
  nk_vc_putchar('\n');
  return 0;
}

#define PRINT_MAX 1024

int nk_vc_printf(char *fmt, ...)
{
  char buf[PRINT_MAX];

  va_list args;
  int i;

  va_start(args, fmt);
  i=vsnprintf(buf,PRINT_MAX,fmt,args);
  va_end(args);
  nk_vc_print(buf);
  return i;
}

int nk_vc_printf_specific(struct nk_virtual_console *vc, char *fmt, ...)
{
  char buf[PRINT_MAX];

  va_list args;
  int i;

  va_start(args, fmt);
  i=vsnprintf(buf,PRINT_MAX,fmt,args);
  va_end(args);
  vc_print_specific(vc, buf);
  return i;
}

int nk_vc_log(char *fmt, ...)
{
  char buf[PRINT_MAX];

  va_list args;
  int i;

  va_start(args, fmt);
  i=vsnprintf(buf,PRINT_MAX,fmt,args);
  va_end(args);

  if (!log_vc) {
    // no output to screen possible yet
  } else {
    BUF_LOCK_CONF;
    BUF_LOCK(log_vc);
    _vc_print_specific(log_vc, buf);
    chardev_consoles_print(log_vc, buf);
    BUF_UNLOCK(log_vc);
  }
#ifdef NAUT_CONFIG_VIRTUAL_CONSOLE_SERIAL_MIRROR
  serial_write(buf);
#endif

  return i;
}

static int _vc_setattr_specific(struct nk_virtual_console *vc, uint8_t attr)
{
  vc->cur_attr = attr;
  return 0;
}


int nk_vc_setattr_specific(struct nk_virtual_console *vc, uint8_t attr)
{
  if (vc) {
    BUF_LOCK_CONF;
    BUF_LOCK(vc);
    _vc_setattr_specific(vc,attr);
    BUF_UNLOCK(vc);
  }
  return 0;

}

int nk_vc_setattr(uint8_t attr)
{
  struct nk_virtual_console *vc;
  struct nk_thread *t = get_cur_thread();

  if (!t || !(vc = t->vc)) {
    vc = default_vc;
  }
  if (vc) {
    BUF_LOCK_CONF;
    BUF_LOCK(vc);
    _vc_setattr_specific(vc,attr);
    BUF_UNLOCK(vc);
  }
  return 0;
}

static int _vc_clear_specific(struct nk_virtual_console *vc, uint8_t attr)
{
  int i;

  vc->cur_attr = attr;
  vc->fill_attr = attr;

  for (i = 0; i < vc->buffer->width*vc->buffer->height; i++) {
    vc->buffer->chars[i].symbol = ' ';
    vc->buffer->chars[i].attribute = attr;
  }

  gpudev_consoles_display_buffer(vc, 0);
  gpudev_consoles_flush(vc, 0);

#ifdef NAUT_CONFIG_XEON_PHI
  if (vc==cur_vc) {
    phi_cons_notify_redraw();
  }
#endif

  vc->cursor.x=0;
  vc->cursor.y=0;
  return 0;
}


int nk_vc_clear_specific(struct nk_virtual_console *vc, uint8_t attr)
{
  if (vc) {
    BUF_LOCK_CONF;
    BUF_LOCK(vc);
    _vc_clear_specific(vc,attr);
    BUF_UNLOCK(vc);
  }
  return 0;
}

int nk_vc_clear(uint8_t attr)
{
  BUF_LOCK_CONF;
  struct nk_thread *t = get_cur_thread();
  struct nk_virtual_console *vc;

  if (!t || !(vc = t->vc)) {
    vc = default_vc;
  }

  if (!vc) {
    return 0;
  }

  BUF_LOCK(vc);
  _vc_clear_specific(vc,attr);
  BUF_UNLOCK(vc);
  return 0;
}

#define NAUT_CONFIG_MAX_PRE_VC 4
static int pre_vcs_registered = 0;

static void(*pre_vc_putchar_ptr[NAUT_CONFIG_MAX_PRE_VC])(char);
static void(*pre_vc_puts_ptr[NAUT_CONFIG_MAX_PRE_VC])(char*);

int nk_pre_vc_register(void(*putchar)(char), void(*puts)(char*)) 
{
  // No real locking because this should be occuring very very early in "init"
  // without threading or multiple processors
  if(pre_vcs_registered >= NAUT_CONFIG_MAX_PRE_VC) {
    return -1;
  }

  int num = pre_vcs_registered;
  pre_vc_putchar_ptr[num] = putchar;
  pre_vc_puts_ptr[num] = puts;
 
  pre_vcs_registered += 1;

  return 0;
}
int _pre_vc_puts(char *s, int i) 
{
  if(pre_vc_puts_ptr[i] != NULL) {
    pre_vc_puts_ptr[i](s);
    return 0;
  } else if(pre_vc_putchar_ptr[i] != NULL) {
    while(*s != '\0') {
      pre_vc_putchar_ptr[i](*s);
      s++;
    }
    return 0;
  } else {
    return -1;
  }
}

int nk_pre_vc_puts(char *s) 
{
  // Print to any registered pre_vc's
  for(int i = 0; i < pre_vcs_registered; i++) {
    _pre_vc_puts(s, i);
  }

  return 0;
}
int _pre_vc_putchar(char c, int i)
{
  if(pre_vc_putchar_ptr[i] != NULL) {
    pre_vc_putchar_ptr[i](c);
    return 0;
  } else if(pre_vc_puts_ptr[i] != NULL) {
    char buf[2];
    buf[0] = c;
    buf[1] = '\0';
    pre_vc_puts_ptr[i](buf);
    return 0;
  } else {
      return -1;
  }
}

int nk_pre_vc_putchar(char c)
{
  // Print to all pre_vc's
  for(int i = 0; i < pre_vcs_registered; i++) {
    _pre_vc_putchar(c, i);
  }

  return 0;
}

// called assuming lock is held
static inline int next_index_on_queue(enum nk_vc_type type, int index)
{
  if (type == RAW) {
    return (index + 1) % Scancode_QUEUE_SIZE;
  } else if (type == COOKED) {
    return (index + 1) % Keycode_QUEUE_SIZE;
  } else {
    ERROR("Using index on a raw, unqueued VC\n");
    return 0;
  }
}

// called assuming lock is held
static inline int is_queue_empty(struct nk_virtual_console *vc)
{
  return vc->head == vc->tail;
}

// called assuming lock is held
static inline int is_queue_full(struct nk_virtual_console *vc)
{
  return next_index_on_queue(vc->type, vc->tail) == vc->head;
}

// called without lock
int nk_enqueue_scancode(struct nk_virtual_console *vc, nk_scancode_t scan)
{
  QUEUE_LOCK_CONF;

  QUEUE_LOCK(vc);

  if(vc->type != RAW || is_queue_full(vc)) {
    DEBUG("Cannot enqueue scancode 0x%x (queue is %s)\n",scan, is_queue_full(vc) ? "full" : "not full");
    QUEUE_UNLOCK(vc);
    return -1;
  } else {
    vc->keyboard_queue.s_queue[vc->tail] = scan;
    vc->tail = next_index_on_queue(vc->type, vc->tail);
    QUEUE_UNLOCK(vc);
    nk_wait_queue_wake_all(vc->waiting_threads);
    return 0;
  }
}

int nk_enqueue_keycode(struct nk_virtual_console *vc, nk_keycode_t key)
{
  QUEUE_LOCK_CONF;

  QUEUE_LOCK(vc);

  if(vc->type != COOKED || is_queue_full(vc)) {
    DEBUG("Cannot enqueue keycode 0x%x (queue is %s)\n",key,is_queue_full(vc) ? "full" : "not full");
    QUEUE_UNLOCK(vc);
    return -1;
  } else {
    vc->keyboard_queue.k_queue[vc->tail] = key;
    vc->tail = next_index_on_queue(vc->type, vc->tail);
    QUEUE_UNLOCK(vc);
    nk_wait_queue_wake_all(vc->waiting_threads);
    return 0;
  }
}

nk_scancode_t nk_dequeue_scancode(struct nk_virtual_console *vc)
{
  QUEUE_LOCK_CONF;

  QUEUE_LOCK(vc);

  if(vc->type != RAW || is_queue_empty(vc)) {
    DEBUG("Cannot dequeue scancode (queue is %s)\n",is_queue_empty(vc) ? "empty" : "not empty");
    QUEUE_UNLOCK(vc);
    return NO_SCANCODE;
  } else {
    nk_scancode_t result;
    result = vc->keyboard_queue.s_queue[vc->head];
    vc->head = next_index_on_queue(vc->type, vc->head);
    QUEUE_UNLOCK(vc);
    return result;
  }
}

nk_keycode_t nk_dequeue_keycode(struct nk_virtual_console *vc)
{
  QUEUE_LOCK_CONF;

  QUEUE_LOCK(vc);

  if (vc->type != COOKED || is_queue_empty(vc)) {
    DEBUG("Cannot dequeue keycode (queue is %s)\n",is_queue_empty(vc) ? "empty" : "not empty");
    QUEUE_UNLOCK(vc);
    return NO_KEY;
  } else {
    nk_keycode_t result;
    result = vc->keyboard_queue.k_queue[vc->head];
    vc->head = next_index_on_queue(vc->type, vc->head);
    QUEUE_UNLOCK(vc);
    return result;
  }
}

static int check(void *state)
{
    struct nk_virtual_console *vc = (struct nk_virtual_console *) state;

    // this check is done without a lock since user of nk_vc_wait() will
    // do a final check anyway

    return !is_queue_empty(vc);
}

void nk_vc_wait()
{
  struct nk_virtual_console *vc = get_cur_thread()->vc;

  if (!vc) {
    vc = default_vc;
  }

  nk_wait_queue_sleep_extended(vc->waiting_threads, check, vc);
}


nk_keycode_t nk_vc_get_keycode(int wait)
{
  struct nk_virtual_console *vc = get_cur_thread()->vc;

  if (!vc) {
    vc = default_vc;
  }

  if (vc->type != COOKED) {
    ERROR("Incorrect type of VC for get_keycode\n");
    return NO_KEY;
  }

  while (1) {
    nk_keycode_t k = nk_dequeue_keycode(vc);
    if (k!=NO_KEY) {
      DEBUG("Returning keycode 0x%x\n",k);
      return k;
    }
    if (wait) {
      nk_vc_wait();
    } else {
      return NO_KEY;
    }
  }
}

int nk_vc_getchar_extended(int wait)
{
  nk_keycode_t key;

  while (1) {
    key = nk_vc_get_keycode(wait);

    switch (key) {
    case KEY_UNKNOWN:
    case KEY_LCTRL:
    case KEY_RCTRL:
    case KEY_LSHIFT:
    case KEY_RSHIFT:
    case KEY_PRINTSCRN:
    case KEY_LALT:
    case KEY_RALT:
    case KEY_CAPSLOCK:
    case KEY_F1:
    case KEY_F2:
    case KEY_F3:
    case KEY_F4:
    case KEY_F5:
    case KEY_F6:
    case KEY_F7:
    case KEY_F8:
    case KEY_F9:
    case KEY_F10:
    case KEY_F11:
    case KEY_F12:
    case KEY_NUMLOCK:
    case KEY_SCRLOCK:
    case KEY_KPHOME:
    case KEY_KPUP:
    case KEY_KPMINUS:
    case KEY_KPLEFT:
    case KEY_KPCENTER:
    case KEY_KPRIGHT:
    case KEY_KPPLUS:
    case KEY_KPEND:
    case KEY_KPDOWN:
    case KEY_KPPGDN:
    case KEY_KPINSERT:
    case KEY_KPDEL:
    case KEY_SYSREQ:
      DEBUG("Ignoring special key 0x%x\n",key);
      continue;
      break;
    default: {
      int c = key & 0xff;
      if (c=='\r') {
	c='\n';
      }
      DEBUG("Regular key 0x%x ('%c')\n", c, c);
      return c;
    }
    }
  }
}

int nk_vc_getchar()
{
  return nk_vc_getchar_extended(1);
}

int nk_vc_gets (char *buf, int n, int display, int(*notifier)(char *, void*, int), void *priv)
{
    int i;
    int c;

start:

    for (i = 0; i < n-1; i++) {

        buf[i] = nk_vc_getchar();

        if (buf[i] == ASCII_BS) {

            buf[i--] = 0; // kill the backspace

            if (i < 0) {
                goto start;
            }

            buf[i--] = 0; // kill the prev char

            if (display) {
                nk_vc_putchar(ASCII_BS);
            }

            continue;
        }

        if (buf[i] == '\t') {
            int skipped = notifier ? notifier(buf, priv, i) : 0;
            i += skipped;
            continue;
        }

        if (display) {
            nk_vc_putchar(buf[i]);
            if (notifier && buf[i] != '\n') {
                notifier(buf, priv, i);
            }
        }


        if (buf[i] == '\n') {
            buf[i] = 0;
            return i;
        }

    }

    buf[n-1]=0;

    return n-1;
}

nk_scancode_t nk_vc_get_scancode(int wait)
{
  struct nk_virtual_console *vc = get_cur_thread()->vc;

  if (!vc) {
    vc = default_vc;
  }

  if (vc->type != RAW) {
    ERROR("Incorrect type of VC for get_scancode\n");
    return NO_SCANCODE;
  }

  while (1) {
    nk_scancode_t s = nk_dequeue_scancode(vc);
    if (s!=NO_SCANCODE) {
      return s;
    }
    if (wait) {
      nk_vc_wait();
    } else {
      return NO_SCANCODE;
    }
  }
}

static int enqueue_scancode_as_keycode(struct nk_virtual_console *__cur_vc, uint8_t scan)
{
#ifdef NAUT_CONFIG_ARCH_X86
  nk_keycode_t key = kbd_translate(scan);
#else
  nk_keycode_t key = scan;
#endif
  if(key != NO_KEY) {
    nk_enqueue_keycode(__cur_vc, key);
  }
  return 0;
}

int nk_vc_handle_keyboard(nk_scancode_t scan)
{
  if (!up) {
    return 0;
  }
  DEBUG("Input: %x\n",scan);
  switch (cur_vc->type) {
  case RAW_NOQUEUE:
    DEBUG("Delivering event to console %s via callback\n",cur_vc->name);
    if (cur_vc->ops && cur_vc->ops->raw_noqueue) {
	    cur_vc->ops->raw_noqueue(scan, cur_vc->ops_priv);
    }
    return 0;
    break;
  case RAW:
    return nk_enqueue_scancode(cur_vc, scan);
    break;
  case COOKED:
    return enqueue_scancode_as_keycode(cur_vc, scan);
    break;
  default:
    ERROR("vc %s does not have a valid type\n",cur_vc->name);
    break;
  }
  return 0;
}

int nk_vc_handle_mouse(nk_mouse_event_t *m)
{
  if (!up) {
    return 0;
  }
  // mouse events are not currently routed
  DEBUG("Discarding mouse event\n");
  DEBUG("Mouse Packet: buttons: %s %s %s\n",
	m->left ? "down" : "up",
	m->middle ? "down" : "up",
	m->right ? "down" : "up");
  DEBUG("Mouse Packet: dx: %d dy: %d res: %u\n", m->dx, m->dy, m->res );
  return 0;
}

static int num_shells=0;

static void new_shell()
{
    char name[80];
    sprintf(name,"shell-%d",num_shells);
    num_shells++;
    nk_launch_shell(name,0,0,0); // interactive shell
}


static int vc_list_inited=0;

static void list(void *in, void **out)
{
  struct list_head *cur;
  int i;

  DEBUG("VC list thread\n");
  if (!list_vc) {
    ERROR("No virtual console for list..\n");
    return;
  }
    
  DEBUG("naming VC list thread\n");
  if (nk_thread_name(get_cur_thread(),"vc-list")) {
    ERROR_PRINT("Cannot name vc-list's thread\n");
    return;
  }

  DEBUG("binding VC list thread\n");
  if (nk_bind_vc(get_cur_thread(), list_vc)) {
    ERROR("Cannot bind virtual console for list\n");
    return;
  }

  DEBUG("declaring VC list thread is up\n");
  // declare we are up
  atomic_add(vc_list_inited,1);
  DEBUG("list thread up\n");

  while (1) {
    nk_vc_clear(0xf9);

    nk_vc_print("List of VCs (space to regenerate, plus for new shell)\n\n");

    i=0;
    list_for_each(cur,&vc_list) {
      nk_vc_printf("%c : %s\n", 'a'+i, list_entry(cur,struct nk_virtual_console, vc_node)->name);
      i++;
    }

    int c = nk_vc_getchar();

    if (c=='+') {
	new_shell();
	continue;
    }

    i=0;
    list_for_each(cur,&vc_list) {
      if (c == (i+'a')) {
	nk_switch_to_vc(list_entry(cur,struct nk_virtual_console, vc_node));
	break;
      }
      i++;
    }
  }
}


int nk_switch_to_vc_list()
{
  return nk_switch_to_vc(list_vc);
}

static int start_list()
{

  if (nk_thread_start(list,
                      0,
                      0,
                      1,
                      VC_LIST_STACK_SIZE,
                      &list_tid,
#if defined(NAUT_CONFIG_ARCH_RISCV) || defined(NAUT_CONFIG_ARCH_ARM64)
                      -1
#else
                      0
#endif
      )) {
    ERROR("Failed to launch VC list\n");
    return -1;
  } else {
    DEBUG("Launched VC list\n");
  }

  while (!atomic_add(vc_list_inited,0)) {
      // wait for vc list to be up
      nk_yield();
  }

  INFO("List launched\n");

  return 0;
}


static int char_dev_write_all(struct nk_char_dev *dev,
			      uint64_t count,
			      uint8_t *src,
			      nk_dev_request_type_t type)

{
    uint64_t left, cur;

    left = count;

    while (left>0) {
	cur = nk_char_dev_write(dev,left,&(src[count-left]),type);
	if (cur < 0) {
	    return cur;
	} else {
	    left-=cur;
	}
    }

    return 0;
}

#define MAX_MATCHING_CHARDEV_CONSOLES 64
#define MAX_MATCHING_GPUDEV_CONSOLES 64

static void _chardev_consoles_print(struct nk_virtual_console *vc, char *data, uint64_t len)
{
    struct list_head *cur=0;
    struct chardev_console *c;
    struct chardev_console *matching_cdc[MAX_MATCHING_CHARDEV_CONSOLES];
    int match_count = 0;
    int match_cur;

    STATE_LOCK_CONF;

    // search for matching consoles with the global lock held
    // DOING NO OUTPUT AS WE DO SO TO AVOID POSSIBLE DEADLOCK

    STATE_LOCK();

    list_for_each(cur,&chardev_console_list) {
	c = list_entry(cur,struct chardev_console, chardev_node);
	if (c && c->cur_vc == vc) {
	    matching_cdc[match_count++] = c;
	    if (match_count==MAX_MATCHING_CHARDEV_CONSOLES) {
		break;
	    }
	}
    }

    STATE_UNLOCK();

    // now demux the print without the lock held
    // so that recursive invocations (e.g. DEBUG statements) will work

    for (match_cur=0;match_cur<match_count;match_cur++) {
	c = matching_cdc[match_cur];

	uint64_t i;
	for (i=0;i<len;i++) {
	    // The caller may be invoking us with interrupts off and a
	    // lock held.   This is technically incorrect on the part
	    // of the caller, but debugging and other kinds of output
	    // are all over the place, and may be incrementally introduced
	    // during debugging.
	    //
	    // If this happens, a blocking write to a chardev may
	    // cause a sleep, which would reenable interrupts,
	    // resulting in a chardev interrupt handler (or possibly
	    // another thread) then running and attempting to acquire
	    // the same lock, leading to deadlock.  To break this
	    // circular dependency, we can use a non-blocking write if
	    // interrupts are off.  This means we avoid the deadlock
	    // at the potential cost of dropping data.  The following
	    // allows us to select between these options.
	    //
	    // You probably want this set to 0
	    //
#define NONBLOCKING_OUTPUT_ON_INTERRUPTS_OFF 0

#if NONBLOCKING_OUTPUT_ON_INTERRUPTS_OFF
#define OUTPUT(cp) ({ if (irqs_enabled()) { char_dev_write_all(c->dev,1,cp,NK_DEV_REQ_BLOCKING); } else { nk_char_dev_write(c->dev,1,cp,NK_DEV_REQ_NONBLOCKING), 0; } })
#else
#define OUTPUT(cp) (char_dev_write_all(c->dev,1,cp,NK_DEV_REQ_BLOCKING))
#endif

	    if (data[i]=='\n') {
		// translate lf->crlf
		OUTPUT("\r");
	    }
	    OUTPUT(&data[i]);
	}
    }
}

static void chardev_consoles_print(struct nk_virtual_console *vc, char *data)
{
    _chardev_consoles_print(vc,data,strlen(data));
}

static void chardev_consoles_putchar(struct nk_virtual_console *vc, char data)
{
    _chardev_consoles_print(vc,&data,1);
}

static void gpudev_consoles_set_cursor(struct nk_virtual_console *vc, uint32_t x, uint32_t y, const int have_lock) 
{
    nk_gpu_dev_coordinate_t zero_coord = { 0 };
    
    struct list_head *cur=0;
    struct gpudev_console *c;
    struct gpudev_console *matching_gdc[MAX_MATCHING_GPUDEV_CONSOLES];
    int match_count = 0;
    int match_cur;

    STATE_LOCK_CONF;
      
    // search for matching consoles with the global lock held
    // DOING NO OUTPUT AS WE DO SO TO AVOID POSSIBLE DEADLOCK

    if(!have_lock) {
      STATE_LOCK();
    }

    list_for_each(cur,&gpudev_console_list) {
	c = list_entry(cur,struct gpudev_console, gpudev_node);
	if (c && c->cur_vc && (*c->cur_vc) == vc) {
	    matching_gdc[match_count++] = c;
	    if (match_count==MAX_MATCHING_GPUDEV_CONSOLES) {
		break;
	    }
	}
    }

    if(!have_lock) {
      STATE_UNLOCK();
    }

    for(int i = 0; i < match_count; i++) {
        c = matching_gdc[i];
        if(c->box.width <= x ||
           c->box.height <= y) {
            nk_gpu_dev_text_set_cursor(c->dev, &zero_coord, 0);
        } else {
            nk_gpu_dev_coordinate_t coord = {
              .x = c->box.x + x,
              .y = c->box.y + y,
            };
            nk_gpu_dev_text_set_cursor(c->dev, &coord, NK_GPU_DEV_TEXT_CURSOR_ON);
        }
    }
}

static void gpudev_consoles_set_char(struct nk_virtual_console *vc, uint32_t x, uint32_t y, char symbol, uint8_t attr) 
{
    nk_gpu_dev_char_t character = {
        .symbol = symbol,
        .attribute = attr,
    };
    
    struct list_head *cur=0;
    struct gpudev_console *c;
    struct gpudev_console *matching_gdc[MAX_MATCHING_GPUDEV_CONSOLES];
    int match_count = 0;
    int match_cur;

    STATE_LOCK_CONF;
      
    // search for matching consoles with the global lock held
    // DOING NO OUTPUT AS WE DO SO TO AVOID POSSIBLE DEADLOCK

    STATE_LOCK();

    list_for_each(cur,&gpudev_console_list) {
	c = list_entry(cur,struct gpudev_console, gpudev_node);
	if (c && c->cur_vc && (*c->cur_vc) == vc) {
	    matching_gdc[match_count++] = c;
	    if (match_count==MAX_MATCHING_GPUDEV_CONSOLES) {
		break;
	    }
	}
    }

    STATE_UNLOCK();

    for(int i = 0; i < match_count; i++) {
        c = matching_gdc[i];
        if(c->box.width <= x ||
           c->box.height <= y) {
            // Outside of this gpudev console
            continue;
        } else {
            nk_gpu_dev_coordinate_t coord = {
                .x = c->box.x + x,
                .y = c->box.y + y,
            };
            nk_gpu_dev_text_set_char(c->dev, &coord, &character);
        }
    }
}

static void gpudev_consoles_flush(struct nk_virtual_console *vc, int have_lock) 
{
    struct list_head *cur=0;
    struct gpudev_console *c;
    struct gpudev_console *matching_gdc[MAX_MATCHING_GPUDEV_CONSOLES];
    int match_count = 0;
    int match_cur;

    STATE_LOCK_CONF;
      
    // search for matching consoles with the global lock held
    // DOING NO OUTPUT AS WE DO SO TO AVOID POSSIBLE DEADLOCK

    if(!have_lock) {
      STATE_LOCK();
    }

    list_for_each(cur,&gpudev_console_list) {
	c = list_entry(cur,struct gpudev_console, gpudev_node);
	if (c && c->cur_vc && (*c->cur_vc) == vc) {
	    matching_gdc[match_count++] = c;
	    if (match_count==MAX_MATCHING_GPUDEV_CONSOLES) {
		break;
	    }
	}
    }

    if(!have_lock) {
      STATE_UNLOCK();
    }

    for(int i = 0; i < match_count; i++) {
        c = matching_gdc[i];
        nk_gpu_dev_flush(c->dev);
    }
}

static void gpudev_consoles_display_buffer(struct nk_virtual_console *vc, int have_lock) 
{
    struct list_head *cur=0;
    struct gpudev_console *c;
    struct gpudev_console *matching_gdc[MAX_MATCHING_GPUDEV_CONSOLES];
    int match_count = 0;
    int match_cur;

    STATE_LOCK_CONF;
      
    // search for matching consoles with the global lock held
    // DOING NO OUTPUT AS WE DO SO TO AVOID POSSIBLE DEADLOCK

    if(!have_lock) {
      STATE_LOCK();
    }

    list_for_each(cur,&gpudev_console_list) {
	c = list_entry(cur,struct gpudev_console, gpudev_node);
	if (c && c->cur_vc && (*c->cur_vc) == vc) {
	    matching_gdc[match_count++] = c;
	      if (match_count==MAX_MATCHING_GPUDEV_CONSOLES) {
		  break;
	    }
	}
    }

    if(!have_lock) {
      STATE_UNLOCK();
    }

    for(int i = 0; i < match_count; i++) {
        c = matching_gdc[i];
        nk_gpu_dev_text_set_box_from_charmap(c->dev, &c->box, vc->buffer);
    }
}

static void vc_copy_to_chardev_console(struct chardev_console *c) 
{
  // When we switch VC's try to output the buffer so we can potentially see past output
  // (this is a rough system but it's for debugging so any output at all is appreciated)

  uint32_t height = c->cur_vc->buffer->height;
  uint32_t width = c->cur_vc->buffer->width;

  for(int y = 0; y < height && y <= c->cur_vc->cursor.y; y++) {
    //Remove extra whitespace off the end of the line
    int found_non_whitespace = 0;
    uint32_t eol = width;
    while(eol > 0 && !found_non_whitespace) {
        char chr = NK_GPU_DEV_CHARMAP_CHAR(c->cur_vc->buffer,eol,y).symbol;
        switch(chr) {
            case '\n':
            case '\r':
            case '\t':
            case ' ':
            case '\0':
                eol--;
                break;
            default:
                found_non_whitespace = 1;
                break;
        }
    }
    for(int x = 0; x < eol; x++) {
      if((y == height-1 || y >= c->cur_vc->cursor.y) && x >= c->cur_vc->cursor.x)
      {
        break;
      }
      char chr = NK_GPU_DEV_CHARMAP_CHAR(c->cur_vc->buffer, x, y).symbol;
      if(chr != '\n') {
	    char_dev_write_all(c->dev,1,&chr,NK_DEV_REQ_BLOCKING);
      } else {
        break;
      }
    }
    if(y < height-1 && y < c->cur_vc->cursor.y)
    {
      uint8_t bp = '\r';
      char_dev_write_all(c->dev,1,&bp,NK_DEV_REQ_BLOCKING);
      bp = '\n';
      char_dev_write_all(c->dev,1,&bp,NK_DEV_REQ_BLOCKING);
    }
  }
}

static int chardev_console_handle_input(struct chardev_console *c, uint8_t data)
{
    if (c->cur_vc->type == COOKED) {
	// translate CRLF => LF
	if (data=='\r') {
	    return 0; // ignore
	} else {
	    int ret = nk_enqueue_keycode(c->cur_vc,data);
            if(ret) {
              printk("Failed to enqueue keycode: %c\n", data);
            }
            return ret;
	}
    } else {
	// raw data not currently handled
	return 0;
    }
}

//
// Commands:
//
//   ``1 = left
//   ``2 = right
//   ``3 = vc list
static void chardev_console(void *in, void **out)
{
    uint8_t data[3];
    char myname[80];
    uint8_t cur=0;

    struct chardev_console *c = (struct chardev_console *)in;

    printk("Trying to find chardev: %s\n", c->name);
    c->dev = nk_char_dev_find(c->name);

    if (!c->dev) {
	ERROR("Unable to open char device %s - no chardev console started\n",c->name);
	return;
    }

    strcpy(myname,c->name);
    strcat(myname,"-console");

    if (nk_thread_name(get_cur_thread(),myname)) {
        ERROR_PRINT("Cannot name chardev console's thread\n");
        return;
    }

    // declare we are up
    atomic_add(c->inited,1);


    char buf[80];

    snprintf(buf,80,"\r\n*** Console %s // prev=``1 next=``2 list=``3 ***\r\n",myname);

    char_dev_write_all(c->dev,strlen(buf),buf,NK_DEV_REQ_BLOCKING);

    snprintf(buf,80,"\r\n*** %s ***\r\n",c->cur_vc->name);
    char_dev_write_all(c->dev,strlen(buf),buf,NK_DEV_REQ_BLOCKING);

 top:
    cur = 0;
    while (atomic_add(c->shutdown,0) == 0) {
	if (nk_char_dev_read(c->dev,1,&data[cur],NK_DEV_REQ_BLOCKING)!=1) {
	    cur = 0;
	    // note that we could get kicked out here if we are being asked
	    // to shut down, which will be detected once the loop iterates
	    // Note that this is ignoring other error handling
	    // important not to do I/O here since we could have a dead connection
	    goto top;
	}


	if (cur==0) {
	    if (data[cur]!='`') {
		chardev_console_handle_input(c,data[0]);
	    } else {
		cur++;
	    }
	    continue;
	}

	if (cur==1) {
	    if (data[cur]!='`') {
		chardev_console_handle_input(c,data[0]);
		chardev_console_handle_input(c,data[1]);
		cur=0;
	    } else {
		cur++;
	    }
	    continue;
	}

	if (cur==2) {
	    struct list_head *cur_node = &c->cur_vc->vc_node;
	    struct list_head *next_node = &c->cur_vc->vc_node;
	    int do_data=0;


	    switch (data[cur]) {
	    case '1':
                if (cur_node->prev != &vc_list) {
		    next_node = cur_node->prev;
                }
		break;
	    case '2':
                if (cur_node->next != &vc_list) {
		    next_node = cur_node->next;
                }
		break;
	    case '3': {
		// display the vcs
		struct list_head *cur_local_vc;
		int i;
		char which;
		strcpy(buf,"\r\nList of VCs (+ = new shell)\r\n\r\n");
		char_dev_write_all(c->dev,strlen(buf),buf,NK_DEV_REQ_BLOCKING);
		// ideally this would be done with the state lock held...
		i=0;
		list_for_each(cur_local_vc,&vc_list) {
		    snprintf(buf,80,"%c : %s\r\n", 'a'+i, list_entry(cur_local_vc,struct nk_virtual_console, vc_node)->name);
		    char_dev_write_all(c->dev,strlen(buf),buf,NK_DEV_REQ_BLOCKING);
		    i++;
		}
		// get user input
		while (1) {
		    if (nk_char_dev_read(c->dev,1,&which,NK_DEV_REQ_BLOCKING)!=1) {
			// a disconnect at this point should kick us back to the top
			// to check for a shutdown
			goto top;
		    }
		    if ((which=='+') || (which>='a' && which<('a'+i))) {
			break;
		    } else {
			continue;
		    }
		}
		if (which=='+') {
		    new_shell();
		} else {
		    i=0;
		    list_for_each(cur_local_vc,&vc_list) {
			if (which == (i+'a')) {
			    next_node = cur_local_vc;
			    break;
			}
			i++;
		    }
		}
	    }
		break;
	    default:
		do_data = 1;
		next_node = cur_node;
	    }

	    c->cur_vc = container_of(next_node, struct nk_virtual_console, vc_node);

	    if (do_data) {
		chardev_console_handle_input(c,data[0]);
		chardev_console_handle_input(c,data[1]);
		chardev_console_handle_input(c,data[2]);
	    } else {
		char buf[80];

		snprintf(buf,80,"\r\n*** %s ***\r\n",c->cur_vc->name);

		char_dev_write_all(c->dev,strlen(buf),buf,NK_DEV_REQ_BLOCKING);

	    }

            vc_copy_to_chardev_console(c);

	    cur = 0;

	    continue;
	}
    }
    DEBUG("exiting console thread\n");
    atomic_and(c->inited,0);
}

int nk_vc_start_chardev_console(const char *chardev)
{
    struct chardev_console *c = malloc(sizeof(*c));

    if (!c) {
	ERROR("Cannot allocate chardev console for %s\n",chardev);
	return -1;
    }

    memset(c,0,sizeof(*c));
    strncpy(c->name,chardev,DEV_NAME_LEN);
    c->name[DEV_NAME_LEN-1] = 0;
    c->cur_vc = default_vc;

    // make sure everyone sees this is zeroed...
    atomic_and(c->inited,0);

    if (nk_thread_start(chardev_console, c, 0, 1, CHARDEV_CONSOLE_STACK_SIZE, &c->tid, -1)) {
	ERROR("Failed to launch chardev console handler for %s\n",c->name);
	free(c);
	return -1;
    }

    while (!atomic_add(c->inited,0)) {
	nk_yield();
	// wait for console thread to start
    }

    STATE_LOCK_CONF;

    STATE_LOCK();
    list_add_tail(&c->chardev_node, &chardev_console_list);
    STATE_UNLOCK();

    INFO("chardev console %s launched\n",c->name);

    return 0;
}

int nk_vc_stop_chardev_console(char *chardev)
{
    STATE_LOCK_CONF;
    struct chardev_console *c = 0;
    struct list_head *cur;
    int found=0;

    // find the console
    STATE_LOCK();

    list_for_each(cur,&chardev_console_list) {
	c = list_entry(cur,struct chardev_console, chardev_node);
	if (c && !strcmp(c->name,chardev)) {
	    found = 1;
	    break;
	}
    }

    if (found) {
	list_del_init(&c->chardev_node);
    }

    STATE_UNLOCK();

    if (found) {
	atomic_or(c->shutdown,1);
	// kick it to make sure it sees the shutdown
	nk_dev_signal((struct nk_dev *)(c->dev));
	// wait for it to ack exit
	while (atomic_add(c->inited,0)!=0) {}
	DEBUG("console %s has stopped\n",c->name);
	free(c);
    }

    return 0;
}

#define MAX_GPU_MODES_TO_SEARCH 16
static int
_configure_gpudev_console(struct gpudev_console *c) 
{
    int res;
    nk_gpu_dev_video_mode_t  modes[MAX_GPU_MODES_TO_SEARCH];
    int num_modes = MAX_GPU_MODES_TO_SEARCH;

    res = nk_gpu_dev_get_available_modes(c->dev, modes, &num_modes);
    if(res) {
        return res;
    }

    int best_index = -1;
    for(int i = 0; i < num_modes; i++) {
        if(modes[i].type != NK_GPU_DEV_MODE_TYPE_TEXT) {
            continue;
        }
        int text_channel = -1;
        for(int channel = 0; channel < 4; channel++) {
            if(modes[i].channel_offset[channel] == NK_GPU_DEV_CHANNEL_OFFSET_TEXT) {
                text_channel = channel;
                break;
            }
        }
        if(text_channel < 0) {
            // No text channel
            continue;
        }

        if(best_index < 0) {
            best_index = i;
            continue;
        }
        
        // Compare this mode with our "best" found so far
        // (Currently just look for the mode with the greatest number of vertical lines)
        if(modes[best_index].height < modes[i].height) {
            best_index = i;
            continue;
        }
    }

    if(best_index < 0) {
        // There's no valid text mode to use for a gpudev_console
        return -EINVAL;
    }

    // Set the GPU into the desired text mode
    c->mode = modes[best_index];
    res = nk_gpu_dev_set_mode(c->dev, &c->mode);
    if(res) {
        return res;
    }

    c->box.x = 0;
    c->box.y = 0;
    c->box.width = c->mode.width;
    c->box.height = c->mode.height;

    return 0;
}

int nk_vc_start_gpudev_console(const char *name) 
{
    int res;
    struct gpudev_console *c = malloc(sizeof(*c));

    if (!c) {
  	  ERROR("Cannot allocate gpudev console for %s\n",name);
	  return -ENOMEM;
    }

    memset(c,0,sizeof(*c));
    strncpy(c->name,name,DEV_NAME_LEN);
    c->name[DEV_NAME_LEN-1] = 0;

    c->private_vc = default_vc;
    c->cur_vc = &c->private_vc;

    struct nk_gpu_dev *gpudev = nk_gpu_dev_find(c->name);
    if(gpudev == NULL) {
        ERROR("Failed to find gpu device: \"%s\" when starting gpudev console!\n", name);
        free(c);
        return -ENXIO;
    }
    c->dev = gpudev;

    res = _configure_gpudev_console(c);
    if(res) {
        ERROR("Failed to configure gpudev console \"%s\"!\n", name);
        free(c);
        return res;
    }

    STATE_LOCK_CONF;

    STATE_LOCK();
    list_add_tail(&c->gpudev_node, &gpudev_console_list);
    STATE_UNLOCK();

    INFO("gpudev console %s launched\n",c->name);

    return 0;
}

int nk_vc_make_gpudev_console_current(const char *name) {
    STATE_LOCK_CONF;
    STATE_LOCK();
     
    struct list_head *cur=0;
    struct gpudev_console *c;
    struct gpudev_console *found = NULL;

    list_for_each(cur,&gpudev_console_list) 
    {
      c = list_entry(cur,struct gpudev_console, gpudev_node);
	  if (c && (strcmp(c->name, name) == 0)) {
          found = c;
          break;
	  }
    }

    if(found == NULL) {
        STATE_UNLOCK();
        return -ENXIO;
    }

    c->cur_vc = &cur_vc;

    STATE_UNLOCK();
    return 0;
}

int nk_vc_make_gpudev_console_private(const char *name) 
{
    STATE_LOCK_CONF;
    STATE_LOCK();
     
    struct list_head *cur=0;
    struct gpudev_console *c;
    struct gpudev_console *found = NULL;

    list_for_each(cur,&gpudev_console_list) 
    {
      c = list_entry(cur,struct gpudev_console, gpudev_node);
	  if (c && (strcmp(c->name, name) == 0)) {
          found = c;
          break;
	  }
    }

    if(found == NULL) {
        STATE_UNLOCK();
        return -ENXIO;
    }

    c->cur_vc = &c->private_vc;

    STATE_UNLOCK();
    return 0;
}

int nk_vc_stop_gpudev_console(const char *name) 
{
    STATE_LOCK_CONF;
    struct gpudev_console *c = 0;
    struct list_head *cur;
    int found=0;

    // find the console
    STATE_LOCK();

    list_for_each(cur,&gpudev_console_list) {
	c = list_entry(cur,struct gpudev_console, gpudev_node);
	if (c && !strcmp(c->name,name)) {
	    found = 1;
	    break;
	}
    }

    if (found) {
	list_del_init(&c->gpudev_node);
    }

    STATE_UNLOCK();

    if(found) {
	  free(c);
    }

    return 0;
}

int nk_vc_init(void)
{
  INFO("init\n");
  spinlock_init(&state_lock);
  INIT_LIST_HEAD(&vc_list);
  INIT_LIST_HEAD(&chardev_console_list);
  INIT_LIST_HEAD(&gpudev_console_list);

  default_vc = nk_create_vc("default", COOKED, 0x0f, 0, 0);
  if(!default_vc) {
    ERROR("Cannot create default console...\n");
    return -1;
  }

  log_vc = nk_create_vc("system-log", COOKED, 0x0a, 0, 0);
  if(!log_vc) {
    ERROR("Cannot create log console...\n");
    return -1;
  }

  //default_vc = log_vc;

  list_vc = nk_create_vc("vc-list", COOKED, 0xf9, 0, 0);
  if(!list_vc) {
    ERROR("Cannot create vc list console...\n");
    return -1;
  }

  DEBUG("starting vc-list\n");
  if (start_list()) {
    ERROR("Cannot create vc list thread\n");
    return -1;
  } else {
    DEBUG("Created vc list thread\n");
  }

  cur_vc = default_vc;
  //copy_display_to_vc(cur_vc);
  //DEBUG("Copied display to vc\n");

  cur_vc->cursor.x = 0;
  cur_vc->cursor.y = 0;

#if NAUT_CONFIG_XEON_PHI
  phi_cons_get_cursor(&(cur_vc->cursor.x), &(cur_vc->cursor.y));
  phi_cons_clear_screen();
  phi_cons_set_cursor(cur_vc->cursor.x, cur_vc->cursor.y);
#endif

  gpudev_consoles_set_cursor(cur_vc, cur_vc->cursor.x, cur_vc->cursor.y, 0);
  gpudev_consoles_flush(cur_vc, 0);

  up=1;

  DEBUG("End nk_vc_init\n");
  return 0;
}


int nk_vc_deinit()
{
  struct list_head *cur;

  nk_thread_destroy(list_tid);

  // deactivate VC
  cur_vc = 0;

  list_for_each(cur,&vc_list) {
    if (nk_destroy_vc(list_entry(cur,struct nk_virtual_console, vc_node))) {
      ERROR("Failed to destroy all VCs\n");
      return -1;
    }
  }

  // should free chardev consoles here...

  return 0;
}

nk_declare_sched_init(nk_vc_init);

#ifdef NAUT_CONFIG_VIRTUAL_CONSOLE_CHARDEV_CONSOLE

static int launch_chardev_console(void) {
  if(!nk_vc_is_active()) {
      return -EINIT_DEFER;
  }
  return nk_vc_start_chardev_console(NAUT_CONFIG_VIRTUAL_CONSOLE_CHARDEV_CONSOLE_NAME);
}
nk_declare_launch_init(launch_chardev_console);

#endif

