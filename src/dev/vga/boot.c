
#include <dev/vga_early.h>
#include <nautilus/errno.h>
#include <nautilus/gpudev.h>
#include <nautilus/init.h>
#include <nautilus/printk.h>
#include <nautilus/vc.h>

static nk_gpu_dev_video_mode_t
boot_vga_text_mode_80_25 = {
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
boot_vga_dev_get_available_modes(void *state, nk_gpu_dev_video_mode_t modes[], uint32_t *num) 
{
    if(*num < 1) {
        *num = 0;
    } else {
        *num = 1;
        modes[0] = boot_vga_text_mode_80_25;
    }
    return 0;
}
static int
boot_vga_dev_get_mode(void *state, nk_gpu_dev_video_mode_t *mode) {
    if(mode) {
        *mode = boot_vga_text_mode_80_25;
    }
    return 0;
}
static int
boot_vga_dev_set_mode(void *state, nk_gpu_dev_video_mode_t *mode) {
    if(memcmp(mode, &boot_vga_text_mode_80_25, sizeof(nk_gpu_dev_video_mode_t))) {
        // Not exactly the same
        return 1;
    }
    return 0;
}
static int
boot_vga_dev_flush(void *state) {
    // Don't need to do anything
    return 0;
}
static int
boot_vga_dev_text_set_char(void *state, nk_gpu_dev_coordinate_t *location, nk_gpu_dev_char_t *val) {
    vga_write_screen(location->x, location->y, vga_make_entry(val->symbol, val->attribute));
    return 0;
}
static int
boot_vga_dev_text_set_cursor(void *state, nk_gpu_dev_coordinate_t *location, uint32_t flags) {
    if((flags & NK_GPU_DEV_TEXT_CURSOR_ON) == 0) {
        // TODO: we can't hide the cursor yet
        return -EUNIMPL;
    }
    if((flags & NK_GPU_DEV_TEXT_CURSOR_BLINK) != 0) {
        // TODO: we can't handle blinking the cursor yet
        return -EUNIMPL;
    } 
    vga_set_cursor(location->x, location->y);
    return 0;
}

struct nk_gpu_dev_int boot_vga_dev_int = {
    .get_available_modes = boot_vga_dev_get_available_modes,
    .get_mode = boot_vga_dev_get_mode,
    .set_mode = boot_vga_dev_set_mode,
    .flush = boot_vga_dev_flush,
    .text_set_char = boot_vga_dev_text_set_char,
    .text_set_cursor = boot_vga_dev_text_set_cursor,
};

static int
boot_vga_dev_init(void) {
    printk("boot_vga_dev_init!\n");

    struct nk_gpu_dev *dev = nk_gpu_dev_register("boot-vga",0,&boot_vga_dev_int,NULL);
    if(dev == NULL) {
        return -ENOMEM;
    }

    return 0;
}

static int
boot_vga_gpudev_console_launch(void) {
    int res = nk_vc_start_gpudev_console("boot-vga");
    if(res) {
        return res;
    }
    return nk_vc_make_gpudev_console_current("boot-vga");
}

nk_declare_driver_init(boot_vga_dev_init);
nk_declare_launch_init(boot_vga_gpudev_console_launch);

