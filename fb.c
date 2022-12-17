#include "fb.h"
#include "assert.h"
#include "mailbox.h"

#define db_mode fb.virtual_height == 2 * fb.height

typedef struct {
    unsigned int width;       // width of the physical screen, number of pixels per row
    unsigned int height;      // height of the physical screen
    unsigned int virtual_width;  // width of the virtual framebuffer
    unsigned int virtual_height; // height of the virtual framebuffer // pitch = width * bit_depth/8
    unsigned int pitch;       // number of bytes per row
    unsigned int bit_depth;   // number of bits per pixel
    unsigned int x_offset;    // x of the upper left corner of the virtual fb
    unsigned int y_offset;    // y of the upper left corner of the virtual fb
    void *framebuffer;        // pointer to the start of the framebuffer
    unsigned int total_bytes; // total number of bytes in the framebuffer
} fb_config_t;

// fb is volatile because the GPU will write to it
static volatile fb_config_t fb __attribute__ ((aligned(16)));

void fb_init(unsigned int width, unsigned int height, unsigned int depth_in_bytes, fb_mode_t mode)
{
    // TODO: extend this function to support double-buffered mode

    fb.width = width;
    fb.virtual_width = width;
    fb.height = height;
    fb.virtual_height = height;
    if (mode) fb.virtual_height *= 2;
    fb.bit_depth = depth_in_bytes * 8; // convert number of bytes to number of bits
    fb.x_offset = 0;
    fb.y_offset = 0;

    // the manual states that we must set these value to 0
    // the GPU will return new values in its response
    fb.pitch = 0;
    fb.framebuffer = 0;
    fb.total_bytes = 0;

    // Send address of fb struct to the GPU as message
    bool mailbox_success = mailbox_request(MAILBOX_FRAMEBUFFER, (unsigned int)&fb);
    assert(mailbox_success); // confirm successful config
}


void fb_swap_buffer(void)
{
    // only swap in double buffer mode
    if (db_mode) {
        if (fb.y_offset == fb.height) fb.y_offset = 0;
        else if (fb.y_offset == 0) fb.y_offset = fb.height;
        bool mailbox_success = mailbox_request(MAILBOX_FRAMEBUFFER, (unsigned int)&fb);
        assert(mailbox_success);
    }
}

void* fb_get_draw_buffer(void)
{   // bytes per row times bytes in height
    unsigned int offset = fb.y_offset;
    if (db_mode) {
        if (!offset) offset = fb.height;
        else offset = 0;
    }
    unsigned char* drawbuffer = (unsigned char* )fb.framebuffer + (fb.pitch * offset * fb.bit_depth / 8) / sizeof(unsigned char*);
    return drawbuffer;
}

unsigned int fb_get_width(void)
{
    return fb.width;
}

unsigned int fb_get_height(void)
{
    return fb.height;
}

unsigned int fb_get_depth(void)
{
    // return depth in bytes
    return fb.bit_depth / 8;
}

unsigned int fb_get_pitch(void)
{
    return fb.pitch;
}

