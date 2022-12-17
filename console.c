#include "console.h"
#include <stdarg.h>
#include "strings.h"
#include "printf.h"
#include "gl.h"

// Please use this amount of space between console rows
const static int LINE_SPACING = 5;
static int line_height;

color_t bg_clr;
color_t fg_clr;

char* text; // copy of fb, cast to 2D array for scrolling

unsigned int cursor_x;
unsigned int cursor_y;

unsigned int max_chars;
unsigned int fb_w;
unsigned int fb_h;
unsigned int c_w;
unsigned int c_h;

void console_init(unsigned int nrows, unsigned int ncols, color_t foreground, color_t background)
{
    max_chars = ncols * nrows;

    line_height = gl_get_char_height() + LINE_SPACING;
    // changed to init based on line height 
    gl_init(ncols * gl_get_char_width(), nrows * line_height, GL_DOUBLEBUFFER);

    cursor_x = 0;
    cursor_y = 0;
    fb_w = gl_get_width();
    fb_h = gl_get_height();
    c_w = gl_get_char_width();
    c_h = gl_get_char_height();
    bg_clr = background;
    fg_clr = foreground;
    gl_clear(background);
    gl_swap_buffer();
    gl_clear(background);
}

void console_clear(void)
{
    gl_clear(bg_clr);
    gl_swap_buffer();
    gl_clear(bg_clr);
    cursor_x = 0;
    cursor_y = 0;
}

// ineffecient but oh well
void scroll(void) {
    // copies each row of pixel to last row of pixel
    for (int y = 0; y < fb_h - line_height; y++) {
        for (int x = 0; x < fb_w; x++) {
            gl_draw_pixel(x, y, gl_read_pixel(x, y + line_height));
        }
    }
}

// TODO: implement this helper function (recommended)
    // if ordinary char: insert ch into contents at current cursor position
    // and advance cursor
    // else if special char, do special handling
static void process_char(char ch)
{
    if (ch == '\n') { // wrap if \n 
        cursor_x = 0;
        cursor_y += line_height;
    }
    else if (ch == '\t') { 
        cursor_x += 3 * c_w;
    }
    else if (ch == '\f') { // clear content
        gl_clear(bg_clr);
        gl_swap_buffer();
        gl_clear(bg_clr);
    }
    else if (ch == '\b') {
        if (cursor_x <= 0) { // backspace limited to line
            cursor_x = 0;
        }
        else { // erase one char
            cursor_x -= c_w;
            gl_draw_rect(cursor_x, cursor_y, c_w, c_h, bg_clr);
            gl_swap_buffer();
            gl_draw_rect(cursor_x, cursor_y, c_w, c_h, bg_clr);
        }
    }
    else { // ordinary char
        gl_draw_char(cursor_x, cursor_y, ch, fg_clr);
        gl_swap_buffer();
        gl_draw_char(cursor_x, cursor_y, ch, fg_clr);
        cursor_x += c_w;
    }
    if (cursor_x >= fb_w) { // wrap if x overflow
        cursor_x = 0;
        cursor_y += line_height;
    }
    if (cursor_y >= fb_h) { // vertical scroll if y overflow
        // scroll and clear last row
        scroll(); 
        gl_draw_rect(0, cursor_y - line_height, fb_w, line_height, bg_clr);
        gl_swap_buffer();
        scroll();
        gl_draw_rect(0, cursor_y - line_height, fb_w, line_height, bg_clr);

        cursor_x = 0;
        cursor_y -= line_height;
    }
}

// handling only a single line, then backspace, then multiple rows, then scrolling.
int console_printf(const char *format, ...)
{
    // TODO: implement this function, be sure to use your vsnprintf!
    char temp[max_chars];

    va_list ap;
    va_start(ap, format);
    vsnprintf(temp, max_chars, format, ap); // fills temp w/ formatted str
    va_end(ap);

    for (int i = 0; i < strlen(temp); i++) {
        //gl_swap_buffer();
        process_char(temp[i]);
    }
    
    return strlen(temp);
}

