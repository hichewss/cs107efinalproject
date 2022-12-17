#include "uart.h"
#include "mymodule.h"
#include "gl.h"
#include "console.h"
#include "malloc.h"
#include "strings.h"
#include "shell.h" 
#include "keyboard.h"
#include "strings.h"
#include "timer.h" 
#include "rand.h" 
#include "nunchuck.h"
#include "gpio.h"


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

// Letter (range from 0-25)
unsigned int letter = 0; 

// For formatting purposes an odd number of columns is ideal
unsigned int ROWS = 32; 
unsigned int COLS = 35; 

// Is the game
unsigned int start_game(void)
{

	console_printf("Nice shooting!");
	return rand() % 9999; 

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
		free((void *) top_scorers[9]);
		total--;
	}

	if (index < total) {
		for (int i = total; i > index; i--) { // shift all elements over by one
			top_scorers[i] = top_scorers[i - 1]; 
			top_score_vals[i] = top_score_vals[i - 1]; 
		}
	}

}

static char *strndup(const char *src, size_t n) // taken from lab4
{

	char* ptr = malloc(n + 1); 
	ptr = (char*) memset(ptr, 0x77, n); 
	memcpy(ptr, src, n); 
	ptr[n] = '\0';
	return ptr;

}

unsigned int padding(void)
{

	return COLS - DASHES - LEADERBOARD_LEN;

}

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

void newline(void)
{

	console_printf("\n"); 

}

// Displays the top 10 scorers of the game
void leaderboard(void)
{ 
	draw_dash(COLS);
	newline(); 
	console_printf("|");
	spacing(padding()); 
	console_printf("LEADERBOARD");
	spacing(padding()); 
	console_printf("|\n");
	draw_dash(COLS);
	newline(); 
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
	// determine user's placement
	unsigned int placement = index(min, score); 
	console_printf("\n\nYOU'VE MADE THE TOP 10!\n"); 
	console_printf("\nINPUT USERNAME (MAX 5 CHARS): \n\n");
	console_printf("   >"); 
	char username[USERNAME_LEN]; 
	shell_readline(username, sizeof(username)); // call shell_readline
	adjust_leaderboard(placement); // adjust leaderboard if necessary

	// insert new user into leaderboard
	top_scorers[placement] = strndup(username, strlen(username)); 
	top_score_vals[placement] = score;

}

// Start screen for Aim Lab
void display_menu(void) 
{

	console_clear(); 
	unsigned int pad = COLS - AIMLAB_LEN;
	for (int i = 0; i < ROWS / 8; i++) {
		newline();  
	}
	spacing(pad); 
	console_printf("AIM LAB");
	for (int j = 0; j < ROWS / 2; j++) {
		newline();  
	}
	const char *press = "PRESS ANYWHERE TO START";
	unsigned int press_len = strlen(press); 
	pad = COLS - press_len; 
	spacing(pad);
	// Create blinking mechanism
	unsigned int count = 0; 
	while (1) {
		console_printf("%s", press);
		timer_delay_ms(500);
		console_printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b"); 
		console_printf("                       "); 
		console_printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b"); 
		timer_delay_ms(500);
		count++; 
		if (count == 10) {
			break;
		}
	}

}

void draw_squares_half(unsigned int top_bot)
{
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

	if (top_bot == 0) {
		draw_dash(COLS / 2); 
		console_printf(" "); 
		draw_dash(COLS / 2); 
		newline();
	}

}

unsigned int padding_display(const char *s)
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
	const char *score_write = " SCORE:";
	console_printf("%s %d", score_write, score);
	unsigned int pad = padding_display(score_write);
	if (score >= 1000) pad--;
	spacing(pad * 2); 
	console_printf("| |");
	const char *clicks_write = " CLICKS:";
	console_printf("%s %d", clicks_write, num_clicks);
	pad = padding_display(clicks_write) - 1;
	if (num_clicks < 1000) pad++; 
	if (num_clicks < 100) pad++; 
	spacing(pad * 2);
	console_printf("|\n"); 
	draw_squares_half(top_bot); 
	top_bot++;
	newline();
	draw_squares_half(top_bot);
	top_bot--;
	const char *accuracy_write = " ACCURACY:"; 
	console_printf("|");
	unsigned int accuracy = (int) (((float) num_hits / num_clicks) * 100.0); 
	console_printf("%s %d%%", accuracy_write, accuracy); 
	pad = padding_display(accuracy_write) - 1;
	if (accuracy == 100) pad--; 
	if (accuracy < 10) pad++; 
	spacing(pad * 2); 
	console_printf("| |");
	const char *hits_write = " HITS:"; 
	console_printf("%s %d", hits_write, num_hits);
	pad = padding_display(hits_write) - 1;
	if (num_hits < 1000) pad++; 
	if (num_hits < 100) pad++; 
	spacing(pad * 2);
	console_printf("|\n"); 
	draw_squares_half(top_bot); 
	timer_delay(5);
	newline();
	newline(); 
	const char *click = "CLICK ANYWHERE TO CONTINUE"; 
	pad = COLS - strlen(click); 
	spacing(pad); 
	console_printf("%s", click); 

}

void main(void)
{
	uart_init();
	timer_init(); 
	nunchuck_init();
	while (1) {
		timer_delay_ms(500);
		nunchuck_read_event();
	}
	keyboard_init(GPIO_PIN5, GPIO_PIN6); 
	shell_init(keyboard_read_next, console_printf);
	console_init(ROWS, COLS, GL_WHITE, GL_BLACK); 
	// Enter the game
	while (1) {
		display_menu(); 
		console_clear(); 
		// Obtain user results
		game_data result = start_game();
		unsigned int score = result.hits * 50; 
		timer_delay(3);
		display_end(score, result.hits, result.clicks);
		timer_delay(3); 
		unsigned int min = threshold(top_score_vals);
		if (score >= min) { 
			input_username(min, score);
			console_printf("\nWELL PLAYED!\n"); 
			timer_delay(3);
		}
		console_clear(); 
		// Display leaderboard
		leaderboard();
		timer_delay(3);
		if (total < 10) total++; 
	}

}
