#include "printf.h"
#include "uart.h"
#include "gpio.h"
#include "console.h"
#include "mouse.h"
#include "timer.h"
#include "nunchuck.h"
#include "rand.h"
#include "console.h"
#include "malloc.h"
#include "strings.h"
#include "shell.h" 
#include "keyboard.h"
#include "sensitivity.h"

#define GL_TURQUOISE 0x3EA39C 
#define GL_DARKBLUE 0x232191 
#define GL_CARDINALRED 0xBD2031
#define GL_DARKRED 0x7d0006
#define GL_PALERED 0xff3b3b
#define targetHit gl_read_pixel(cursor_x, cursor_y) == t_clr && gl_read_pixel(cursor_x + 1, cursor_y) == t_clr && gl_read_pixel(cursor_x, cursor_y + 1) == t_clr && gl_read_pixel(cursor_x + 1, cursor_y + 1) == t_clr

static int cursor_x = 100;
static int cursor_y = 100;

static int targets[10]; // 5 targets, tracks top left coordinates of target, x: even index, y: odd
static unsigned int t_rad = 16; // target radius
static unsigned int t_pts = 50; // target points
static color_t t_clr = GL_TURQUOISE; // target color
static color_t crosshair_clr = GL_WHITE; // crosshair
static color_t bg_clr = GL_BLACK; // background
static color_t text_clr = GL_WHITE; // text
static color_t HUD_clr = GL_DARKBLUE; // hud color

static color_t ex_clr_up[6];
static color_t ex_clr_down[6];
static color_t ex_clr_horizontal[16];

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
static unsigned int GL_PADDING = 10;

typedef struct {
    unsigned int clicks;
    unsigned int hits;
} game_data;

int unsigned_to_base(char* buf, size_t bufsize, unsigned int val, int base, size_t
    min_width);

#define MAX_OUTPUT_LEN 1024
#define USERNAME_LEN 6
#define DASHES 2
#define LEADERBOARD_LEN 11
#define SCORE_LEN 4
#define AIMLAB_LEN 7

// Leaderboard global vars
const char* top_scorers[10];
int top_score_vals[10];
// number of top scorers
unsigned int top_scorers_num = 0;
// Leaderboard total 
unsigned int total = 0;

// For formatting purposes an odd number of columns is ideal
unsigned int ROWS = 32;
unsigned int COLS = 35;

//static color_t draw_clr[14] = { GL_BLACK, GL_RED, GL_GREEN, GL_DARKBLUE, GL_CYAN,
//                           GL_MAGENTA, GL_YELLOW, GL_AMBER, GL_ORANGE,
//                           GL_PURPLE, GL_INDIGO, GL_CAYENNE, GL_MOSS, GL_SILVER };

static void update_cursor(int dx, int dy) {
    cursor_x += dx; // move x, y
    cursor_y -= dy; // y is inverted

    // keep within bounds
    if (cursor_x > GL_WIDTH - GL_PADDING) cursor_x = GL_WIDTH - GL_PADDING;
    if (cursor_x <= GL_PADDING) cursor_x = GL_PADDING;
    if (cursor_y > GL_HEIGHT - GL_PADDING) cursor_y = GL_HEIGHT - GL_PADDING;
    if (cursor_y <= GL_HEIGHT - spawn_h - GL_PADDING) cursor_y = GL_HEIGHT - spawn_h - GL_PADDING;
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
    while (x + t_rad * 2 >= spawn_w || y + t_rad * 2 >= spawn_h || overlap(x + GL_PADDING, y + GL_PADDING + text_height * 2, t_rad, t_clr)) {
        x = rand() % spawn_w;
        y = rand() % spawn_h;
    }

    x += GL_PADDING;
    y += GL_PADDING + text_height * 2;
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
    // draw top triangle
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
    unsigned int timer_w = 4 * text_width + 2 * GL_PADDING;
    unsigned int timer_h = ui_h + 4;
    timer_x = (GL_WIDTH - timer_w) / 2;
    HUD_text_y = GL_PADDING + (ui_h - text_height) / 2 + 2;

    // hud band
    draw_diamond(x - ui_h / 2, GL_PADDING, ui_h, HUD_clr);
    gl_draw_rect(x, GL_PADDING, ui_w, ui_h, HUD_clr);
    draw_diamond(x + ui_w - ui_h / 2, GL_PADDING, ui_h, HUD_clr);

    // timer countdown
    draw_diamond(timer_x - timer_h / 2, GL_PADDING - 2, timer_h, t_clr);
    gl_draw_rect(timer_x, GL_PADDING - 2, timer_w, timer_h, t_clr);
    draw_diamond(timer_x + timer_w - timer_h / 2, GL_PADDING - 2, timer_h, t_clr);
    gl_draw_string(timer_x + GL_PADDING, HUD_text_y, "1:00", text_clr);

    // score left
    score_x = x + text_width;
    gl_draw_char(score_x, HUD_text_y, '0', text_clr);

    // accuracy right
    acc_x = timer_x + timer_w + GL_PADDING * 3;
    gl_draw_string(acc_x, HUD_text_y, "100%", text_clr);
}

//store color of cursor position
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

// the actual game program.
// returns game_data
static game_data start_game() {

	//initialize status to be returned to the endcard/leaderboard
	game_data results = { 0, 0 };
	char score[10];
	size_t scoresize = sizeof(score);
	char accuracy[4];
	size_t accuracysize = sizeof(accuracy);
	char countdown[4];
	size_t countdownsize = sizeof(countdown);

	//set up top screen with score, timer, and accuracy
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

	//variable used to track press/releases of the trigger button (BZ)
	unsigned int pressed = 0;

	//variables used to increment crosshair/cursor
	int dx = 0;
	int dy = 0;

	while (timer > 0) {
		timer_delay_ms(5);
		cur_time = 60 - ((timer_get_ticks() / 1000000) - init_time);
		// update countdown timer display
		if (cur_time < timer) {
			timer = cur_time;
			gl_draw_rect(timer_x + GL_PADDING, HUD_text_y, text_width * 4, text_height, t_clr);
			unsigned_to_base(countdown, countdownsize, timer, 10, 2);
			gl_draw_string(timer_x + GL_PADDING, HUD_text_y, "0:", text_clr);
			gl_draw_string(timer_x + GL_PADDING + text_width * 2, HUD_text_y, countdown, text_clr);
		}
		// pull the nunchuck data to update cursor & check for shots
		nunchuck_event_t* action = nunchuck_read_event();

		//load cursor data
		dx = action->dx;
		dy = action->dy;

		if (dx || dy) { // moved
			reset_crosshair_pixels(); //set cur crosshair pixels to old clr
			update_cursor(logb(dx, 10), logb(dy, 10));
			fill_ex_clr();
			draw_crosshair(); // draw crosshair
		}

		if (action->BZ && !pressed) { // shot fired
			pressed = 1; // set flag to prevent false retriggers
			results.clicks++; //data to calculate accuracy in endscreen/leaderboard

			if (targetHit) { // target hit
				results.hits++; //endscreen data

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

		//re enable trigger/shot once the button has been released
		} else if (!action->BZ) {
			pressed = 0;
		
		}
	}
	return results; //send back game data
}

// Determines the minimum threshold needed to make the top 10 scorers
unsigned int threshold(int arr[])
{
	int result = 0;
	if (total == 0) return result;

	result = arr[0];
	for (int i = 1; i <= total; i++) {
		if (arr[i] < result) result = arr[i];
	}
	return result;
}

// Find the user placement on the leaderboard
unsigned int index(unsigned int min, unsigned int score)
{
	unsigned int index = total;
	for (int i = total - 1; i >= 0; i--) {
		if (score >= top_score_vals[i]) {
			index--; // if score is greater, move up one place
		}
	}
	return index;
}

// Adjust the leaderboard when a user makes the top 10
void adjust_leaderboard(unsigned int index)
{
	// Free memory of last place scorer
	if (total == 10) {
		free((void*)top_scorers[9]);
		total--;
	}

	if (index < total) {
		for (int i = total; i > index; i--) { // shift all elements over by one
			top_scorers[i] = top_scorers[i - 1];
			top_score_vals[i] = top_score_vals[i - 1];
		}
	}
}

static char* strndup(const char* src, size_t n) // taken from lab4
{

	char* ptr = malloc(n + 1);
	ptr = (char*)memset(ptr, 0x77, n);
	memcpy(ptr, src, n);
	ptr[n] = '\0';
	return ptr;

}

// Calculate spacing needed for leaderboard text
unsigned int padding(void)
{
	return COLS - DASHES - LEADERBOARD_LEN;
}

// Calculate spacing needed for each user
unsigned int padding2(void)
{
	return COLS - DASHES - SCORE_LEN - USERNAME_LEN;
}

// Add amount of padding needed to display leaderboard
void spacing(unsigned int pad)
{
	for (int i = 0; i < pad / 2; i++) {
		console_printf(" ");
	}
}

// Draw leaderboard dashes
void draw_dash(unsigned int num)
{

	for (int i = 0; i < num; i++) {
		console_printf("-");
	}

}

// Print a new line on the console
void newline(void)
{

	console_printf("\n");

}

// Displays the top 10 scorers of the game
void leaderboard(void)
{
        // Draw leaderboard structure in retro style 
	draw_dash(COLS);
	newline();
	console_printf("|");
	spacing(padding());
	console_printf("LEADERBOARD");
	spacing(padding());
	console_printf("|\n");
	draw_dash(COLS);
	newline();
	// Draw out names of all users on the leaderboard
	for (int i = 0; i <= total; i++) {
		unsigned int check = 1000;
		console_printf("|");
		console_printf(top_scorers[i]);
		int len = strlen(top_scorers[i]);
		while (len < USERNAME_LEN - 1) { // username padding
			console_printf(" ");
			len++;
		}
		console_printf("|");
		while (top_score_vals[i] < check) { // score padding
			console_printf(" ");
			check = check / 10;
		}
		unsigned int pad = padding2();
		for (int i = 0; i < pad - 1; i++) { // additional score padding
			console_printf(" ");
		}
		console_printf("%d", top_score_vals[i]);
		console_printf(" |\n");
		draw_dash(COLS);
		newline();
	}

}

// Input username
void input_username(unsigned int min, unsigned int score)
{

	console_clear();
	// Determine user's placement
	unsigned int placement = index(min, score);
	console_printf("\n\nYOU'VE MADE THE TOP 10!\n");
	console_printf("\nINPUT USERNAME (MAX 5 CHARS): \n\n");
	console_printf("  > ");
	char username[USERNAME_LEN];
	shell_readline(username, sizeof(username)); // call shell_readline
	adjust_leaderboard(placement); // adjust leaderboard if necessary

	// insert new user into leaderboard and malloc into memory
	top_scorers[placement] = strndup(username, strlen(username));
	top_score_vals[placement] = score;

}

// Start screen for Aim Lab
void display_menu(void)
{

	console_clear();
	// Format menu text appropriately
	unsigned int pad = COLS - AIMLAB_LEN;
	for (int i = 0; i < ROWS / 8; i++) {
		newline();
	}
	spacing(pad);
	console_printf("AIM LAB"); //text at top of screen
	for (int j = 0; j < ROWS / 2; j++) { //go down to bottom of screen to write bottom text
		newline();
	}
	const char* press = "PRESS Z TO START"; //bottom text (bottom text) *meme*
	unsigned int press_len = strlen(press);
	pad = COLS - press_len;
	spacing(pad);

	// Create blinking mechanism
	unsigned int count = 0;
	nunchuck_event_t *action = nunchuck_read_event(); //start taking input from controller
	while (1) { //this loops every second at 10 fps
		if (!count) { //prints on the first frame
			console_printf("%s", press); //print bottom text
		} else if (count == 5) { //delete bottom text on the sixth frame
			console_printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
			console_printf("                ");
			console_printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
		}
		action = nunchuck_read_event(); //check controller input
		// Proceed to game if user hits 'Z'
		if (action->BZ) {
			break;
		}
		timer_delay_ms(100); //10 fps in this current iteration
		count++; //increment
		count %= 10; //cycle

	}
}

// Draw half squares for each stat on results screen
void draw_squares_half(unsigned int top_bot)
{
	// Draw upper half
	if (top_bot == 1) {
		draw_dash(COLS / 2);
		console_printf(" ");
		draw_dash(COLS / 2);
		newline();
	}

	for (int i = 0; i < ROWS / 16; i++) {
		console_printf("|");
		spacing(COLS - 4);
		console_printf("|");
		console_printf(" |");
		spacing(COLS - 4);
		console_printf("|\n");
	}

	// Draw lower half
	if (top_bot == 0) {
		draw_dash(COLS / 2);
		console_printf(" ");
		draw_dash(COLS / 2);
		newline();
	}

}

// Calculate spacing for each scoring statistic
unsigned int padding_display(const char* s)
{

	return ((COLS - 4) / 2 - SCORE_LEN - strlen(s));

}

// Display game end screen
void display_end(unsigned int score, unsigned int num_hits, unsigned int num_clicks)
{

	console_clear();
	// 1 means draw upper half and 0 means draw lower half
	unsigned int top_bot = 1;
	console_printf("AIM LAB\n\n");

	// Display all score statistics
	draw_squares_half(top_bot);
	top_bot--;
	console_printf("|");
	const char* score_write = " SCORE:";
	console_printf("%s %d", score_write, score);
	unsigned int pad = padding_display(score_write);

	// Account for all edge cases that would throw off formatting
	if (score >= 1000) pad--;
	if (score < 100) pad++;
	if (score < 10) pad++;
	spacing(pad * 2);
	console_printf("| |");

	//display clicks stat box
	const char* clicks_write = " CLICKS:";
	console_printf("%s %d", clicks_write, num_clicks);
	pad = padding_display(clicks_write) - 1;
	//similar edge case
	if (num_clicks < 1000) pad++;
	if (num_clicks < 100) pad++;
	if (num_clicks < 10) pad++;
	spacing(pad * 2);
	console_printf("|\n");

	//go draw squares in the second row now
	draw_squares_half(top_bot);
	top_bot++;
	newline();
	draw_squares_half(top_bot);
	top_bot--;

	// display accuracy stat
	const char* accuracy_write = " ACCURACY:";
	console_printf("|");
	unsigned int accuracy = (int)(((float)num_hits / num_clicks) * 100.0);
	console_printf("%s %d%%", accuracy_write, accuracy);
	pad = padding_display(accuracy_write);
	//edge cases
	if (accuracy == 100) pad--;
	if (accuracy < 10) pad++;
	spacing(pad * 2);
	console_printf("| |");

	//display targets hit stat
	const char* hits_write = " HITS:";
	console_printf("%s %d", hits_write, num_hits);
	pad = padding_display(hits_write) - 1;
	//more edge cases
	if (num_hits < 1000) pad++;
	if (num_hits < 100) pad++;
	if (num_hits < 10) pad++;
	spacing(pad * 2);
	console_printf("|\n");

	//complete the grid by drawing the bottom of the squares
	draw_squares_half(top_bot);
	newline();
	newline();

	//display prompt beneath screen
	const char* click = "CLICK Z TO CONTINUE";
	pad = COLS - strlen(click);	
	spacing(pad);
	unsigned int count = 0;

	nunchuck_event_t *action; //start taking controller input
	// Create blinking mechanism
	while (1) { //this loop also each second, but at 20 fps
		if (!count) { //display text starting on the first frame
			console_printf("%s", click);
		} else if (count == 10) { //clear text on the eleventh frame
			console_printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
			console_printf("                   ");
			console_printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
		}
		timer_delay_ms(50); // 20 fps

		action = nunchuck_read_event(); //read input

		// Move to next screen if user hits 'Z'
		if (action->BZ) {
			break;
		}
		count++; //increment
		count %= 20; //cycle
	}

}

void main(void)
{
	// init
	gpio_init();
	timer_init();
	uart_init();
	nunchuck_init();
	gl_init(GL_WIDTH, GL_HEIGHT, GL_SINGLEBUFFER);

	gl_clear(GL_BLACK);

	keyboard_init(GPIO_PIN5, GPIO_PIN6);
	shell_init(keyboard_read_next, console_printf);

	// global vars
	text_height = gl_get_char_height();
	text_width = gl_get_char_width();
	ui_w = GL_WIDTH / 2;
	ui_h = GL_HEIGHT / 16;
	if (ui_h % 2 == 0) ui_h++; // odd height for ui bar
	spawn_h = GL_HEIGHT - GL_PADDING * 3 - ui_h;
	spawn_w = GL_WIDTH - GL_PADDING * 2;

	//the main program that cycles between game and title/endcard and leaderboard
	while (1) {
		// start screen
		console_init(ROWS, COLS, GL_WHITE, GL_BLACK);
		display_menu();
		console_clear();
		// start game, get game stats
		gl_init(GL_WIDTH, GL_HEIGHT, GL_SINGLEBUFFER);
		gl_clear(GL_BLACK);
		game_data result = start_game();

		//calculate some leaderboard data
		unsigned int score = result.hits * 50;
		timer_delay(3);

		//start endcard
		console_init(ROWS, COLS, GL_WHITE, GL_BLACK);
		display_end(score, result.hits, result.clicks);
		unsigned int min = threshold(top_score_vals);
		if (score >= min) {
			input_username(min, score);
			console_printf("\nWELL PLAYED!\n");
			timer_delay(3);
		}
		console_clear();
		// display leaderboard
		leaderboard();
		timer_delay(3);
		if (total < 10) total++;
	}

	uart_putchar(EOT);
}
