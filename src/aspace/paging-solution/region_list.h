#ifndef __MM_LINKED_LIST_H__
#define __MM_LINKED_LIST_H__

#include <nautilus/aspace.h>
#include <nautilus/nautilus.h>

typedef struct mm_llist_node {
    nk_aspace_region_t region;
    struct mm_llist_node * next_llist_node;
} mm_llist_node_t;

typedef struct mm_llist {
    uint32_t size;
    mm_llist_node_t * region_head;
} mm_llist_t;

mm_llist_t * mm_llist_create();

/*
insert region into the VA-PA map tracking data structure
*/
int mm_insert(mm_llist_t * self, nk_aspace_region_t * region);

/*
check if region overlaps with existing region in the data structure
If overlapped, return the pointer to region in the data structure that overlaps with input region;
otherwise, return NULL;
*/
nk_aspace_region_t * mm_check_overlap(mm_llist_t * self, nk_aspace_region_t * region);

/*
remove region in the data structure that is same as input region in terms of check_flags (See region_equal for details)
if Removal is successful, return 0
otherwise return -1
*/
int mm_remove(mm_llist_t * self, nk_aspace_region_t * region, uint8_t check_flags);

/*
find the ptr to region in the data structure that is same as input region in terms of check_flags (See region_equal for details)
if no region matched the criterion, return NULL
*/
nk_aspace_region_t* mm_contains(mm_llist_t * self, nk_aspace_region_t * region, uint8_t check_flags);

/*
find the ptr to region in the data structure that contains the virtual address speicified
if no region contains such address, return NULL
*/

nk_aspace_region_t * mm_find_reg_at_addr (mm_llist_t * self, addr_t address);

/*
update region in the data structure that matches cur_region in terms of criterion specified by eq_flag. (See region_equal for details)
the matched region in data structure has all fields updated except those specified by eq_flag. (See region_update for details)
*/
nk_aspace_region_t * mm_update_region (
    mm_llist_t * self, 
    nk_aspace_region_t * cur_region, 
    nk_aspace_region_t * new_region, 
    uint8_t eq_flag
);

/*
printout the VA-PA map tracking data structure
*/
void mm_show(mm_llist_t * self);

/*
    destroy and free the data structure
*/

int mm_destory (mm_llist_t * self);


#define VA_CHECK 1
#define PA_CHECK 2
#define LEN_CHECK 4
#define PROTECT_CHECK 8

/*
    return 1 if two input regions are equal in terms of check_flags
    {VA_CHECK|PA_CHECK|LEN_CHECK|PROTECT_CHECK} controls which attribute of input regions are going to be checked
    Typically, we always have VA_CHECK and LEN_CHECK set, but you are free to choose not do it if it fits your requirement. 
*/
int region_equal(nk_aspace_region_t * regionA, nk_aspace_region_t * regionB, uint8_t check_flags);

/*
    update every attribute that's not set up in eq_flags in dest from src.
    Eg. if  eq_flags = VA_CHECK|LEN_CHECK
    we going to have dest->pa_start = src->pa_start and dest->protect = src->protect
*/
int region_update(nk_aspace_region_t * dest, nk_aspace_region_t * src, uint8_t eq_flags);

/*
    check if regionA and regionB overlap. 
    If so return 1, ow. 0
*/
int overlap_helper(nk_aspace_region_t * regionA, nk_aspace_region_t * regionB);

int region2str(nk_aspace_region_t * region,  char * str);

#endif
