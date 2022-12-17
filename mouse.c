#include "mouse.h"
#include "ps2.h"
#include "printf.h"
#include "gpio_interrupts.h"
#include "assert.h"

static ps2_device_t* dev;

// typedef enum {
//    MOUSE_BUTTON_PRESS = 0,
//    MOUSE_BUTTON_RELEASE = 1,
//    MOUSE_MOVED = 2,
//    MOUSE_DRAGGED = 3,
// } mouse_action_t;

// typedef struct {
//    mouse_action_t action;
//    int dx, dy;
//    bool x_overflow, y_overflow;
//    bool left, middle, right;
// } mouse_event_t;


// resets the mouse and enables data reporting mode
void mouse_init(unsigned int clock_gpio, unsigned int data_gpio) {
	printf("mouse init called");

	dev = ps2_new(clock_gpio, data_gpio);

	ps2_write(dev, 0xFF); // Reset
	printf("BAT : %x \n", ps2_read(dev)); 
	printf("ID %x \n", ps2_read(dev));
	ps2_write(dev, 0xF4); // Enable Data Reporting

	printf("mouse init success");
}

mouse_event_t mouse_read_event(void) { 
	mouse_event_t event = {};
	// read 3 bytes total
	unsigned char byte1 = ps2_read(dev);
	printf("byte1 = %d\n", byte1);
	unsigned char xMove = ps2_read(dev); // two's complement
	printf("xMove = %d\n", xMove);
	unsigned char yMove = ps2_read(dev);
	printf("yMove = %d\n", yMove);

	event.x_overflow = byte1 & 1 << 6; // bit 6
	event.y_overflow = byte1 & 1 << 7; // bit 7
	
	event.dx = (byte1 & 1 << 4) ? ~xMove + 1 : xMove;
	event.dy = (byte1 & 1 << 5) ? ~yMove + 1 : yMove;

	event.middle = byte1 & 1 << 2;
	event.right = byte1 & 1 << 1;
	event.left = byte1 & 1 << 0;

	// left button press & button release
	event.action = !event.left;
	if (yMove || xMove) { // moves
		if (event.action == MOUSE_BUTTON_PRESS) event.action = MOUSE_DRAGGED;
		else event.action = MOUSE_MOVED;
	}
	printf("event action = %d\n", event.action);
	return event;
}
