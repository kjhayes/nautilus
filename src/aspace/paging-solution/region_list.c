#include "region_list.h"


// #ifndef NAUT_CONFIG_DEBUG_ASPACE_PAGING
// #undef DEBUG_PRINT
// #define DEBUG_PRINT(fmt, args...) 
// #endif


#define ERROR_LLIST(fmt, args...) ERROR_PRINT("aspace-paging-llist: " fmt, ##args)
#define DEBUG_LLIST(fmt, args...) DEBUG_PRINT("aspace-paging-llist: " fmt, ##args)
#define INFO_LLIST(fmt, args...)   INFO_PRINT("aspace-paging-llist: " fmt, ##args)

int overlap_helper(nk_aspace_region_t * regionA, nk_aspace_region_t * regionB){
    void * VA_start_A = regionA->va_start;
    void * VA_start_B = regionB->va_start;
    void * VA_end_A = regionA->va_start + regionA->len_bytes;
    void * VA_end_B = regionB->va_start + regionB->len_bytes;

    if (VA_start_A <= VA_start_B && VA_start_B < VA_end_A) {
        return 1;
    }
    if (VA_start_B <= VA_start_A && VA_start_A < VA_end_B) {
        return 1;
    }

    return 0;
}

int region_equal(nk_aspace_region_t * regionA, nk_aspace_region_t * regionB, uint8_t check_flags) {
    if (check_flags & VA_CHECK) {
        if (regionA->va_start != regionB->va_start) return 0;
    }

    if (check_flags & PA_CHECK) {
        if (regionA->pa_start != regionB->pa_start) return 0;
    }

    if (check_flags & LEN_CHECK) {
        if (regionA->len_bytes != regionB->len_bytes) return 0;
    }

    if (check_flags & PROTECT_CHECK) {
        if (regionA->protect.flags != regionB->protect.flags) return 0;
    }

    return 1;
}

int region_update(nk_aspace_region_t * dest, nk_aspace_region_t * src, uint8_t eq_flags) {
    if (!(eq_flags & VA_CHECK)) {
        dest->va_start = src->va_start;
    }

    if (!(eq_flags & PA_CHECK)) {
        dest->pa_start = src->pa_start;
    }

    if (!(eq_flags & LEN_CHECK)) {
        dest->len_bytes = src->len_bytes;
    }

    if (!(eq_flags & PROTECT_CHECK)) {
        dest->protect = src->protect;
    }

    return 0;
} 


int mm_insert(mm_llist_t * self, nk_aspace_region_t * region) {
    mm_llist_t * llist = self;

    mm_llist_node_t * newhead = (mm_llist_node_t *) malloc(sizeof(mm_llist_node_t));

    if (!newhead) {
        ERROR_LLIST("cannot allocate a new linked list node!\n");
        panic("cannot allocate a new linked list node!\n");
        return -1;
    }
    
    newhead->region = *region;
    newhead->next_llist_node = llist->region_head;

    llist->region_head = newhead;

    llist->size += 1;
    return 0;
}

void mm_show(mm_llist_t * self) {
    DEBUG_LLIST("Printing regions in linked list\n");

    mm_llist_t * llist = self;

    mm_llist_node_t * curr_node = llist->region_head;
    while (curr_node != NULL)
    {
        DEBUG_LLIST("VA = %016lx to PA = %016lx, len = %16lx\n", 
            curr_node->region.va_start,
            curr_node->region.pa_start,
            curr_node->region.len_bytes
        );
        curr_node = curr_node->next_llist_node;
    }
}

nk_aspace_region_t * mm_check_overlap(mm_llist_t * self, nk_aspace_region_t * region){
    mm_llist_t * llist = self;
    mm_llist_node_t * curr_node = llist->region_head;

    while (curr_node != NULL) {
        nk_aspace_region_t * curr_region_ptr = &curr_node->region;
        if (overlap_helper(curr_region_ptr, region)) {
            return curr_region_ptr;
        }
        curr_node = curr_node->next_llist_node;
    }
    return NULL;
}

int mm_remove(mm_llist_t * self, nk_aspace_region_t * region, uint8_t check_flags){
    mm_llist_t * llist = (mm_llist_t * ) self;
    mm_llist_node_t * curr_node = llist->region_head;
    mm_llist_node_t * prev_node = NULL;

    while (curr_node != NULL) {
        nk_aspace_region_t * curr_region_ptr = &curr_node->region;
        if (region_equal(curr_region_ptr, region, check_flags)) {
            // delete the node
            if (prev_node != NULL) {
                prev_node->next_llist_node = curr_node->next_llist_node;
            } else {
                llist->region_head = curr_node->next_llist_node;
            }
            
            free(curr_node);
            llist->size -= 1;
            return 0;
        }

        prev_node = curr_node;
        curr_node = curr_node->next_llist_node;
    }
    
    return -1;
}

nk_aspace_region_t* mm_contains(mm_llist_t * self, nk_aspace_region_t * region, uint8_t check_flags) {
    mm_llist_t * llist = (mm_llist_t * ) self;
    mm_llist_node_t * curr_node = llist->region_head;

    while (curr_node != NULL) {
        nk_aspace_region_t * curr_region_ptr = &curr_node->region;
        if (region_equal(curr_region_ptr, region, check_flags)) {
            return curr_region_ptr;
        }
        curr_node = curr_node->next_llist_node;
    }
    return NULL;
}

nk_aspace_region_t * mm_find_reg_at_addr (mm_llist_t * self, addr_t address) {
    mm_llist_t * llist = (mm_llist_t * ) self;
    mm_llist_node_t * curr_node = llist->region_head;

    while (curr_node != NULL) {
        nk_aspace_region_t curr_reg = curr_node->region;
        if ( 
            (addr_t) curr_reg.va_start <= address && 
            address < (addr_t) curr_reg.va_start + (addr_t) curr_reg.len_bytes
        ) {
            return &curr_node->region;
        }
        curr_node = curr_node->next_llist_node;
    }
    
    return NULL;
}



nk_aspace_region_t * mm_update_region (
    mm_llist_t * self, 
    nk_aspace_region_t * cur_region, 
    nk_aspace_region_t * new_region, 
    uint8_t eq_flag
) {
    mm_llist_t * llist = (mm_llist_t * ) self;
    mm_llist_node_t * curr_node = llist->region_head;

    while (curr_node != NULL) {
        nk_aspace_region_t * curr_region_ptr = &curr_node->region;

        if (region_equal(curr_region_ptr, cur_region, eq_flag)) {
            region_update(curr_region_ptr, new_region, eq_flag);
            return curr_region_ptr;
        }
        curr_node = curr_node->next_llist_node;
    }
    return NULL;
}

int mm_llist_node_destroy(mm_llist_node_t * node) {
    if (node != NULL) {
        mm_llist_node_destroy(node->next_llist_node);
        free(node);
    }
    return 0;
}

int mm_destory(mm_llist_t * self) {
    mm_llist_t * llist = (mm_llist_t * ) self;
    mm_llist_node_t * head = llist->region_head;
    
    mm_llist_node_destroy(head);

    free(llist);
    
    DEBUG_LLIST("Done: destroy linked list!\n");
    return 0;
}

int mm_llist_init(mm_llist_t * llist) {
    
    llist->size = 0;
    llist->region_head = NULL;

    return 0;
}

mm_llist_t * mm_llist_create() {
    mm_llist_t *mylist = (mm_llist_t *) malloc(sizeof(mm_llist_t));

    if (! mylist) {
        ERROR_LLIST("cannot allocate a linked list data structure to track region mapping\n");
        panic("cannot allocate a linked list data structure to track region mapping\n");
        return 0;
    }

    // mm_llist_init(mylist);
    mm_llist_init(mylist);


    return mylist;
}