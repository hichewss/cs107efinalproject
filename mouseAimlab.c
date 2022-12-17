#include "printf.h"
#include "uart.h"
#include "gpio.h"
#include "console.h"
#include "mouse.h"
#include "timer.h"
#include "rand.h"
#include "interrupts.h"

// felicity.c uses mouse

#define CLOCK GPIO_PIN3
#define DATA GPIO_PIN4
#define GL_TURQUOISE 0x3EA39C 

static int cursor_x = 100;
static int cursor_y = 100;

static unsigned int t_rad = 8; // target radius
static unsigned int t_pts = 50; // target points
static color_t t_clr = GL_RED; // target color
static color_t crosshair_clr = GL_WHITE; // crosshair
static color_t bg_clr = GL_BLACK; // background
static color_t text_clr = GL_WHITE; // text
static color_t HUD_clr = GL_BLUE; // hud color
static color_t ex_clr_up[6];
static color_t ex_clr_down[6];
static color_t ex_clr_horizontal[16];

static int targets[10]; // 5 targets, tracks up left coordinates of target, x: even index, y: odd

typedef struct {
    unsigned int clicks;
    unsigned int hits;
} game_data;

static unsigned int text_height;
static unsigned int text_width;
static unsigned int GL_WIDTH = 520;
static unsigned int GL_HEIGHT = 380;
static unsigned int ui_w;
static unsigned int ui_h;
static unsigned int score_x;
static unsigned int timer_x;
static unsigned int acc_x;
static unsigned int HUD_text_y;
static unsigned int spawn_h; // window targets can spawn in, includes padding
static unsigned int spawn_w;
static unsigned int padding = 10;

int unsigned_to_base(char* buf, size_t bufsize, unsigned int val, int base, size_t
    min_width);

//static color_t draw_clr[14] = { GL_BLACK, GL_RED, GL_GREEN, GL_BLUE, GL_CYAN,
//                           GL_MAGENTA, GL_YELLOW, GL_AMBER, GL_ORANGE,
//                           GL_PURPLE, GL_INDIGO, GL_CAYENNE, GL_MOSS, GL_SILVER };

static void update_cursor(int dx, int dy) {
    cursor_x += dx; // move x, y
    cursor_y -= dy; // y is inverted

    // keep within bounds
    if (cursor_x > GL_WIDTH - padding) cursor_x = GL_WIDTH - padding;
    if (cursor_x <= padding) cursor_x = padding;
    if (cursor_y > GL_HEIGHT - padding) cursor_y = GL_HEIGHT - padding;
    if (cursor_y <= padding) cursor_y = padding;
}

static void draw_circle(int x, int y, int r, color_t c) {
    for (int i = -1 * r; i < r + 1; i++) {
        for (int j = -1 * r; j < r + 1; j++) {
            if (i * i + j * j <= r * r) { // if point is in circle
                gl_draw_pixel(x + i + r, y + j + r, c);
            }
        }
    }
}

// checks if a square at x, y would overlap another circle
static bool overlap(int x, int y, int r, color_t c) {
    for (int i = 0; i < r * 2; i++) {
        for (int j = 0; j < r * 2; j++) {
            if (gl_read_pixel(x + i, y + j) == c) {
                return true;
            }
        }
    }
    return false;
}

// randomly generates a target on screen
static void inst_target(unsigned int index) {
    unsigned int x = rand() % spawn_w;
    unsigned int y = rand() % spawn_h;
    // if overlap another circle or exceed bounds, get new position
    while (x + t_rad * 2 >= spawn_w || y + t_rad * 2 >= spawn_h || overlap(x + padding, y + padding + text_height * 2, t_rad, t_clr)) {
        x = rand() % spawn_w;
        y = rand() % spawn_h;
    }

    x += padding;
    y += padding + text_height * 2;
    draw_circle(x, y, t_rad, t_clr);
    targets[index] = x;
    targets[index + 1] = y;
}

// return index (of x) of corresponding target hit
static unsigned int find_target() {
    // always 10 targets on screen
    for (int i = 0; i < 5; i++) {
        if (cursor_x >= targets[i * 2] && cursor_x < targets[i * 2] + 2 * t_rad && cursor_y >= targets[i * 2 + 1] && cursor_y < targets[i * 2 + 1] + t_rad * 2) {
            return i * 2;
        }
    }
    return 0;
}

static void draw_diamond(int x, int y, unsigned int h, color_t c) {
    h = h / 2 + 1;
    unsigned int n = 1; // pixels drawn on each row
    // draw up triangle
    for (int i = 0; i < h; i++) {
        unsigned int pad = h - i - 2;
        for (int k = 1; k <= n; k++) {
            gl_draw_pixel(x + pad + k, y + i, c);
        }
        n += 2;
    }
    y += h - 1;
    n = 1;
    // draw lower triangle
    for (int i = h - 1; i >= 1; i--) {
        unsigned int pad = i - 1;
        for (int k = 1; k <= n; k++) {
            gl_draw_pixel(x + pad + k, y + i, c);
        }
        n += 2;
    }
}

// PTS 0 <1:00> 100%
static void draw_ui_bar() {
    unsigned int x = (GL_WIDTH - ui_w) / 2;
    unsigned int timer_w = 4 * text_width + 2 * padding;
    unsigned int timer_h = ui_h + 4;
    timer_x = (GL_WIDTH - timer_w) / 2;
    HUD_text_y = padding + (ui_h - text_height) / 2 + 2;

    // hud bad
    draw_diamond(x - ui_h / 2, padding, ui_h, HUD_clr);
    gl_draw_rect(x, padding, ui_w, ui_h, HUD_clr);
    draw_diamond(x + ui_w - ui_h / 2, padding, ui_h, HUD_clr);

    // timer countdown
    draw_diamond(timer_x - timer_h / 2, padding - 2, timer_h, GL_TURQUOISE);
    gl_draw_rect(timer_x, padding - 2, timer_w, timer_h, GL_TURQUOISE);
    draw_diamond(timer_x + timer_w - timer_h / 2, padding - 2, timer_h, GL_TURQUOISE);
    gl_draw_string(timer_x + padding, HUD_text_y, "1:00", text_clr);

    // score left
    score_x = x + text_width;
    gl_draw_char(score_x, HUD_text_y, '0', text_clr);

    // accuracy right
    acc_x = timer_x + timer_w + padding * 3;
    gl_draw_string(acc_x, HUD_text_y, "100%", text_clr);
}

//store clr of cursor position
static void fill_ex_clr() {
    // left center right
    unsigned int i = 0;
    for (int y = 0; y < 2; y++) {
        for (int x = -3; x <= 4; x++) {
            ex_clr_horizontal[i] = gl_read_pixel(cursor_x + x, cursor_y + y);
            i++;
        }
    }
    // up
    i = 0;
    for (int y = -3; y < 0; y++) {
        for (int x = 0; x < 2; x++) {
            ex_clr_up[i] = gl_read_pixel(cursor_x + x, cursor_y + y);
            i++;
        }
    }
    // down
    i = 0;
    for (int y = 2; y < 5; y++) {
        for (int x = 0; x < 2; x++) {
            ex_clr_down[i] = gl_read_pixel(cursor_x + x, cursor_y + y);
            i++;
        }
    }
}

static void draw_crosshair() {
    // draw crosshair
    gl_draw_rect(cursor_x, cursor_y - 3, 2, 3, crosshair_clr); // up
    gl_draw_rect(cursor_x, cursor_y + 2, 2, 3, crosshair_clr); // down
    gl_draw_rect(cursor_x - 3, cursor_y, 3, 2, crosshair_clr); // left
    gl_draw_rect(cursor_x + 2, cursor_y, 3, 2, crosshair_clr); // right
}

// fill crosshair pixels with ex_clr
static void reset_crosshair_pixels() {
    // left center right
    unsigned int i = 0;
    for (int y = 0; y < 2; y++) {
        for (int x = -3; x <= 4; x++) {
            gl_draw_pixel(cursor_x + x, cursor_y + y, ex_clr_horizontal[i]);
            i++;
        }
    }
    // up
    i = 0;
    for (int y = -3; y < 0; y++) {
        for (int x = 0; x < 2; x++) {
            gl_draw_pixel(cursor_x + x, cursor_y + y, ex_clr_up[i]);
            i++;
        }
    }
    // down
    i = 0;
    for (int y = 2; y < 5; y++) {
        for (int x = 0; x < 2; x++) {
            gl_draw_pixel(cursor_x + x, cursor_y + y, ex_clr_down[i]);
            i++;
        }
    }
}

// returns game_data
static game_data start_game() {
    game_data results = { 0, 0 };
    char score[10];
    size_t scoresize = sizeof(score);
    char accuracy[4];
    size_t accuracysize = sizeof(accuracy);
    char countdown[4];
    size_t countdownsize = sizeof(countdown);

    draw_ui_bar();

    // instantiate n targets & fill array
    for (int i = 0; i < 5; i++) {
        inst_target(i * 2);
        //timer_delay(1);
    }
    fill_ex_clr();

    // gives init time in seconds
    unsigned int init_time = timer_get_ticks() / 1000000;
    unsigned int timer = 60;
    unsigned int cur_time = 60;

    while (timer > 0) {
        cur_time = 60 - ((timer_get_ticks() / 1000000) - init_time);
        // update countdown timer display
        if (cur_time < timer) {
            timer = cur_time;
            gl_draw_rect(timer_x + padding, HUD_text_y, text_width * 4, text_height, GL_TURQUOISE);
            unsigned_to_base(countdown, countdownsize, timer, 10, 2);
            gl_draw_string(timer_x + padding, HUD_text_y, "0:", text_clr);
            gl_draw_string(timer_x + padding + text_width * 2, HUD_text_y, countdown, text_clr);
        }
        mouse_event_t event = mouse_read_event();
        mouse_action_t action = event.action;
        if (action == 2) { //moved
            reset_crosshair_pixels(); //set cur crosshair pixels to old clr
            update_cursor(event.dx, event.dy);
            fill_ex_clr();
            draw_crosshair(); // draw crosshair
        }
        else if (action == 1) { //clicked
            results.clicks++;
            if (gl_read_pixel(cursor_x, cursor_y) == t_clr) { // target hit
                results.hits++;
                unsigned int index = find_target();
                gl_draw_rect(targets[index], targets[index + 1], t_rad * 2 + 1, t_rad * 2 + 1, bg_clr); // erase target
                inst_target(index); // new target
                // redraw crosshair
                fill_ex_clr();
                draw_crosshair();
                // update score 
                gl_draw_rect(score_x, HUD_text_y, text_width * 4, text_height, HUD_clr);
                unsigned_to_base(score, scoresize, results.hits * t_pts, 10, 0);
                gl_draw_string(score_x, HUD_text_y, score, text_clr);
            }
            // update accuracy
            if (results.hits != 0) {
                gl_draw_rect(acc_x, HUD_text_y, text_width * 3, text_height, HUD_clr);
                unsigned_to_base(accuracy, accuracysize, results.hits * 100 / results.clicks, 10, 0);
                gl_draw_string(acc_x, HUD_text_y, accuracy, text_clr);
            }
            else {
                gl_draw_rect(acc_x, HUD_text_y, text_width * 2, text_height, HUD_clr);
            }
        }
    }
    return results;
}

void main(void)
{
    interrupts_init();
    gpio_init();
    timer_init();
    uart_init();
    interrupts_global_enable();

    mouse_init(CLOCK, DATA);
    gl_init(GL_WIDTH, GL_HEIGHT, GL_SINGLEBUFFER);
    gl_clear(GL_BLACK);

    text_height = gl_get_char_height();
    text_width = gl_get_char_width();
    ui_w = GL_WIDTH / 2;
    ui_h = GL_HEIGHT / 16;
    if (ui_h % 2 == 0) ui_h++; // odd height for ui bar
    spawn_h = GL_HEIGHT - padding * 3 - ui_h;
    spawn_w = GL_WIDTH - padding * 2;
    game_data g = start_game();
    printf("clicks: %d, hits: %d, score: %d, accuracy: %d%%\n", g.clicks, g.hits, g.hits * t_pts, 0); // g.hits * 100 /g.clicks

    uart_putchar(EOT);
}
