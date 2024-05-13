/*
 * This file is part of the Nautilus AeroKernel developed
 * by the Constellation, Interweaving, Hobbes, and V3VEE
 * Projects with funding from the United States National
 * Science Foundation and the Department of Energy.
 *
 * The V3VEE Project is a joint project between Northwestern University
 * and the University of New Mexico.  The Hobbes Project is a collaboration
 * led by Sandia National Laboratories that includes several national
 * laboratories and universities. The Interweaving Project is a
 * joint project between Northwestern University and Illinois Institute
 * of Technology.   The Constellation Project is a joint project
 * between Northwestern University, Carnegie Mellon University,
 * and Illinois Institute of Technology.
 * You can find out more at:
 * http://www.v3vee.org
 * http://xstack.sandia.gov/hobbes
 * http://interweaving.org
 * http://constellation-project.net
 *
 * Copyright (c) 2023, Kevin Hayes <kjhayes@u.northwestern.edu>
 * Copyright (c) 2023, The V3VEE Project  <http://www.v3vee.org>
 *                     The Hobbes Project <http://xstack.sandia.gov/hobbes>
 * All rights reserved.
 *
 * Authors: Kevin Hayes <kjhayes@u.northwestern.edu>
 *
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "LICENSE.txt".
 */

#include <dev/vga.h>
#include <nautilus/gpudev.h>
#include <nautilus/errno.h>

#define LOCAL_LOCK_CONF int _local_flags=0
#define LOCAL_LOCK(vga) _local_flags = spin_lock_irq_save(&vga->lock);
#define LOCAL_UNLOCK(vga) spin_unlock_irq_restore(&vga->lock, _local_flags);

int
vga_initialize_struct(struct vga *vga, struct vga_ops *ops)
{
    spinlock_init(&vga->lock);
    vga->vga_ops = *ops;
    return 0;
}

// nk_gpu_dev generic operations

static nk_gpu_dev_video_mode_t
generic_vga_text_mode_80_25 = {
    .type = NK_GPU_DEV_MODE_TYPE_TEXT,
    .width = 80,
    .height = 25,
    .channel_offset = {
        NK_GPU_DEV_CHANNEL_OFFSET_TEXT,
        NK_GPU_DEV_CHANNEL_OFFSET_ATTR,
        -1,
        -1,
    },
    .flags = 0,
    .mouse_cursor_width = 0,
    .mouse_cursor_height = 0,
    .mode_data = NULL,
};

static int
generic_vga_dev_get_available_modes(void *state, nk_gpu_dev_video_mode_t modes[], uint32_t *num) {
    return -EUNIMPL;
}
static int
generic_vga_dev_get_mode(void *state, nk_gpu_dev_video_mode_t *mode) {
    return -EUNIMPL;
}
static int
generic_vga_dev_set_mode(void *state, nk_gpu_dev_video_mode_t *mode) {
    return -EUNIMPL;
}
static int
generic_vga_dev_flush(void *state) {
    return -EUNIMPL;
}
static int
generic_vga_dev_text_set_char(void *state, nk_gpu_dev_coordinate_t *location, nk_gpu_dev_char_t *val) {
    return -EUNIMPL;
}
static int
generic_vga_dev_text_set_cursor(void *state, nk_gpu_dev_coordinate_t *location, uint32_t flags) {
    return -EUNIMPL;
}
static int
generic_vga_dev_graphics_set_clipping_box(void *state, nk_gpu_dev_box_t *box) {
    return -EUNIMPL;
}
static int
generic_vga_dev_graphics_set_clipping_region(void *state, nk_gpu_dev_region_t *region) {
    return -EUNIMPL;
}
static int
generic_vga_dev_graphics_draw_pixel(void *state, nk_gpu_dev_coordinate_t *location, nk_gpu_dev_pixel_t *pixel) {
    return -EUNIMPL;
}
static int
generic_vga_dev_graphics_draw_line(void *state, nk_gpu_dev_coordinate_t *start, nk_gpu_dev_coordinate_t *end, nk_gpu_dev_pixel_t *pixel) {
    return -EUNIMPL;
}
static int
generic_vga_dev_graphics_draw_poly(void *state, nk_gpu_dev_coordinate_t *coord_list, uint32_t count, nk_gpu_dev_pixel_t *pixel) {
    return -EUNIMPL;
}
static int
generic_vga_dev_graphics_fill_box_with_pixel(void *state, nk_gpu_dev_box_t *box, nk_gpu_dev_pixel_t *pixel, nk_gpu_dev_bit_blit_op_t op) {
    return -EUNIMPL;
}
static int
generic_vga_dev_graphics_fill_box_with_bitmap(void *state, nk_gpu_dev_box_t *box, nk_gpu_dev_bitmap_t *bitmap, nk_gpu_dev_bit_blit_op_t op) {
    return -EUNIMPL;
}
static int
generic_vga_dev_graphics_copy_box(void *state, nk_gpu_dev_box_t *source_box, nk_gpu_dev_box_t *dest_box, nk_gpu_dev_bit_blit_op_t op) {
    return -EUNIMPL;
}
static int
generic_vga_dev_graphics_draw_text(void *state, nk_gpu_dev_coordinate_t *location, nk_gpu_dev_font_t *font, char *string) {
    return -EUNIMPL;
}
static int
generic_vga_dev_graphics_set_cursor_bitmap(void *state, nk_gpu_dev_bitmap_t *bitmap) {
    return -EUNIMPL;
}
static int
generic_vga_dev_graphics_set_cursor(void *state, nk_gpu_dev_coordinate_t *location) {
    return -EUNIMPL;
}

struct nk_gpu_dev_int generic_vga_dev_int = {
    .get_available_modes = generic_vga_dev_get_available_modes,
    .get_mode = generic_vga_dev_get_mode,
    .set_mode = generic_vga_dev_set_mode,
    .flush = generic_vga_dev_flush,
    .text_set_char = generic_vga_dev_text_set_char,
    .text_set_cursor = generic_vga_dev_text_set_cursor,
    .graphics_set_clipping_box = generic_vga_dev_graphics_set_clipping_box,
    .graphics_set_clipping_region = generic_vga_dev_graphics_set_clipping_region,
    .graphics_draw_pixel = generic_vga_dev_graphics_draw_pixel,
    .graphics_draw_line = generic_vga_dev_graphics_draw_line,
    .graphics_draw_poly = generic_vga_dev_graphics_draw_poly,
    .graphics_fill_box_with_pixel = generic_vga_dev_graphics_fill_box_with_pixel,
    .graphics_fill_box_with_bitmap = generic_vga_dev_graphics_fill_box_with_bitmap,
    .graphics_copy_box = generic_vga_dev_graphics_copy_box,
    .graphics_draw_text = generic_vga_dev_graphics_draw_text,
};

