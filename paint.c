#include "printf.h"
#include "uart.h"
#include "gpio.h"
#include "console.h"
#include "mouse.h"

#define CLOCK GPIO_PIN3
#define DATA GPIO_PIN4

static int cursor_x = 0;
static int cursor_y = 0;

static color_t draw_clr[14] = {GL_BLACK, GL_RED, GL_GREEN, GL_BLUE, GL_CYAN,
                           GL_MAGENTA, GL_YELLOW, GL_AMBER, GL_ORANGE,
                           GL_PURPLE, GL_INDIGO, GL_CAYENNE, GL_MOSS, GL_SILVER };

static void update_cursor(int dx, int dy) {
    cursor_x += dx; // move x, y
    cursor_y -= dy; // y is inverted

    // keep within bounds
    if (cursor_x > gl_get_width()) cursor_x = gl_get_width() - 1;
    if (cursor_x <= 0) cursor_x = 0;
    if (cursor_y > gl_get_height()) cursor_y = gl_get_height() - 1;
    if (cursor_y <= 0) cursor_y = 0;
}

void main(void) 
{
    interrupts_init();
    gpio_init();
    timer_init();
    uart_init();
    interrupts_global_enable();

    mouse_init(CLOCK, DATA);
    gl_init(200, 200, GL_SINGLEBUFFER);
    gl_clear(GL_WHITE);

    uart_putstring("Hello, world!\n");
    printf("I am printf, here m%c %s!\n", 'e', "ROAR");

    color_t ex_clr = GL_WHITE;
    unsigned int ind = 0;

    // pressed, dragged = pixel color changes
    // moved, release = display pointer
    while (1) {
        mouse_event_t event = mouse_read_event();
        mouse_action_t action = event.action;
        if (action == 2) { //moved
            gl_draw_pixel(cursor_x, cursor_y, ex_clr); //set cur pix to old clr
            update_cursor(event.dx, event.dy);
            ex_clr = gl_read_pixel(cursor_x, cursor_y); //store clr of new cursor position
            gl_draw_pixel(cursor_x, cursor_y, GL_BLACK); // draw pointer
        }
        else if (action == 3 || action == 0) { //dragged or pressed
            update_cursor(event.dx, event.dy);
            gl_draw_pixel(cursor_x, cursor_y, draw_clr[ind]); // draw pixel
        }
        if (event.right) { // right button pressed
            ind++; // cycle through draw_clr
            if (ind >= 14) ind = 0;
        }
        // do nothing for press and release
    }

    uart_putchar(EOT);
}
