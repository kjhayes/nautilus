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
 * Copyright (c) 2016, Peter Dinda <pdinda@northwestern.edu>
 * Copyright (c) 2015, The V3VEE Project  <http://www.v3vee.org> 
 *                     The Hobbes Project <http://xstack.sandia.gov/hobbes>
 * All rights reserved.
 *
 * Author: Peter Dinda <pdinda@northwestern.edu>
 *
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "LICENSE.txt".
 */

#ifndef __DEV
#define __DEV

#include <nautilus/list.h>

#define DEV_NAME_LEN 32
typedef enum {
    NK_DEV_GENERIC, 
    NK_DEV_TIMER,
    NK_DEV_BUS,
    NK_DEV_CHAR, 
    NK_DEV_BLK, 
    NK_DEV_NET,
    NK_DEV_GRAPHICS,
    NK_DEV_SOUND,
    NK_DEV_IRQ,
    NK_DEV_GPIO,
} nk_dev_type_t ; 


// this is the abstract base class for device interfaces
// it should be the first member of any type of specific device interface
struct nk_dev_int {
    int (*open)(void *state);
    int (*close)(void *state);
};

typedef struct nk_wait_queue nk_wait_queue_t;

#define NK_DEV_FLAG_NO_WAIT (1<<0) // you cannot wait on this device (nk_dev_wait will immediately return regardless of context)

// this is the class for devices.  It should be the first
// member of any specific type of device
struct nk_dev {
    char name[DEV_NAME_LEN];
    nk_dev_type_t type; 
    uint64_t      flags;
    struct list_head dev_list_node;
    
    void *state; // driver state
    
    struct nk_dev_int *interface;
    
    nk_wait_queue_t *waiting_threads;
};

// Not all request types apply to all device types
// Not all devices support the request types valid
// for a specific device type
typedef enum {NK_DEV_REQ_BLOCKING, NK_DEV_REQ_NONBLOCKING, NK_DEV_REQ_CALLBACK} nk_dev_request_type_t;

int nk_dev_inited(void);

struct nk_dev *nk_dev_register(char *name, nk_dev_type_t type, uint64_t flags, struct nk_dev_int *inter, void *state);
int nk_dev_register_pre_allocated(struct nk_dev *d, char *name, nk_dev_type_t type, uint64_t flags, struct nk_dev_int *inter, void *state);
int            nk_dev_unregister(struct nk_dev *);
int            nk_dev_unregister_pre_allocated(struct nk_dev *);

struct nk_dev *nk_dev_find(char *name);

void nk_dev_wait(struct nk_dev*, int (*cond_check)(void *state), void *state);
void nk_dev_signal(struct nk_dev *);

void nk_dev_dump_devices();

/*
 * Device Numbering
 *
 * Atomic functions to consistently name devices which might share a name
 */

// Get N for a new "serial<N>" device
uint32_t nk_dev_get_serial_device_number(void);

#endif
