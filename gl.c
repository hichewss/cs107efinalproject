#include "gl.h"
#include "strings.h"
#include "font.h"
#include "printf.h"

#define inBounds (x >= 0 && x < pix_per_row) && (y >= 0 && y < fb_height)
unsigned int pix_per_row;
unsigned int fb_height;

void gl_init(unsigned int width, unsigned int height, gl_mode_t mode)
{
    fb_init(width, height, 4, mode);    // use 32-bit depth always for graphics library
    pix_per_row = fb_get_pitch() / fb_get_depth();
    fb_height = fb_get_height();
}

void gl_swap_buffer(void)
{
    //unsigned char* src = (unsigned char*)fb_get_draw_buffer();
    fb_swap_buffer();
    //unsigned char* dst = (unsigned char*)fb_get_draw_buffer();
    //int nbytes = fb_get_pitch() * fb_get_height();    
    //memcpy(dst, src, nbytes);
}

unsigned int gl_get_width(void)
{
    return fb_get_width();
}

unsigned int gl_get_height(void)
{
    return fb_get_height();
}

color_t gl_color(unsigned char r, unsigned char g, unsigned char b)
{
    // argb
    return 0xff000000 | (r << 16) | (g << 8) | (b << 0);
}

void gl_clear(color_t c)
{
    unsigned int(*im)[pix_per_row] = fb_get_draw_buffer();
    for (int x = 0; x < pix_per_row; x++) {
        for (int y = 0; y < fb_get_height(); y++) {
            im[y][x] = c;
        }
    }
}

void gl_draw_pixel(int x, int y, color_t c)
{
    unsigned int(*im)[pix_per_row] = fb_get_draw_buffer();
    if (inBounds) im[y][x] = c;
}

color_t gl_read_pixel(int x, int y)
{
    //unsigned int pix_per_row = fb_get_pitch() / fb_get_depth(); // length of each row in pixels (include any padding)
    unsigned int(*im)[pix_per_row] = fb_get_draw_buffer();
    if (inBounds) return im[y][x];
    return 0;
}

void gl_draw_rect(int x, int y, int w, int h, color_t c)
{
    for (int i = x; i < x + w; i++) {
        for (int j = y; j < y + h; j++) {
            gl_draw_pixel(i, j, c);
        }
    }
}

void gl_draw_char(int x, int y, char ch, color_t c)
{
    unsigned char buf[font_get_glyph_size()];
    // fill buf w char bits & check valid
    if (font_get_glyph(ch, buf, sizeof(buf))) {
        unsigned int gwidth = font_get_glyph_width();
        unsigned int gheight = font_get_glyph_height();
        unsigned char(*img)[gwidth] = (unsigned char(*)[gwidth])buf;

        for (int i = 0; i < gheight; i++) { //y
            for (int j = 0; j < gwidth; j++) { //x
                if (img[i][j] == 0xff) {
                    gl_draw_pixel(j + x, i + y, c);
                }
            }
        }
    }
}

void gl_draw_string(int x, int y, const char* str, color_t c)
{
    char* s = (char*)str;
    unsigned int w = font_get_glyph_width();
    unsigned int h = font_get_glyph_height();
    // in bounds and has item in string
    while (*s != '\0' && x < (int) pix_per_row) {
        gl_draw_char(x, y, *s, c);
        x+=w;
        s++;
    }
}

unsigned int gl_get_char_height(void)
{
    return font_get_glyph_height();
}

unsigned int gl_get_char_width(void)
{
    return font_get_glyph_width();
}
