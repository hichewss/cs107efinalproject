#include "gpio.h"
#include "gpio_extra.h"
#include "gpio_interrupts.h"
#include "malloc.h"
#include "ps2.h"
#include "uart.h"
#include "timer.h"
#include "ringbuffer.h"

static unsigned int bit = 0; // tracks which bit currently on
static unsigned int pCount = 0; // parity counter

// This struct definition fully defines the type declared in ps2.h
// Since the ps2.h API only uses this struct as a pointer, its
// definition can be implementation-specific and deferred until
// here.
struct ps2_device {
    unsigned int clock; // GPIO pin number of the clock line
    unsigned int data;  // GPIO pin number of the data line
    rb_t* rb; // ringbuffer
    unsigned int scancode; // 11 scan bits
};

void handler(unsigned int pc, void* aux_data) {
    //uart_putchar('+');
    ps2_device_t* dev = (ps2_device_t*)aux_data;
    unsigned int curBit = gpio_read(dev->data);
    // if start bit not 0 or parity wrong or stop bit, reset all to 0
    if ((bit == 0 && curBit != 0) || (bit == 9 && !((pCount + curBit) % 2)) || (bit == 10)) {
        if (bit == 10 && curBit) rb_enqueue(dev->rb, dev->scancode);
        bit = 0;
        pCount = 0;
        dev->scancode = 0;
    }
    else {
        if (bit >= 1 && bit <= 8) { // bits 1-8
            dev->scancode += curBit << (bit - 1);
            if (curBit) pCount++; // if 1
        }
        bit++;
    }
    gpio_clear_event(dev->clock);
}

ps2_device_t* ps2_new(unsigned int clock_gpio, unsigned int data_gpio)
{
    // consider why must malloc be used to allocate device
    ps2_device_t* dev = malloc(sizeof(*dev));

    dev->rb = rb_new();

    dev->clock = clock_gpio;
    gpio_set_input(dev->clock);
    gpio_set_pullup(dev->clock);

    dev->data = data_gpio;
    gpio_set_input(dev->data);
    gpio_set_pullup(dev->data);

    gpio_enable_event_detection(dev->clock, GPIO_DETECT_FALLING_EDGE);
    gpio_interrupts_init();    
    gpio_interrupts_register_handler(dev->clock, handler, dev);
    gpio_interrupts_enable();
    
    return dev;
}

void wait_for_falling_clock_edge(ps2_device_t* dev)
{
    while (gpio_read(dev->clock) == 0) {}
    while (gpio_read(dev->clock) == 1) {}
    //return gpio_read(dev->data);
}

unsigned char ps2_read(ps2_device_t* dev)
{
    int scancode = 0;
//    int start_time = timer_get_ticks();
    while (1) {
        if (dev->rb && rb_dequeue(dev->rb, &scancode)) { // dequeue when scancode avail
            return (unsigned char) scancode;
//        } else if (timer_get_ticks() - start_time > 50000) {
//		return 
    }
}

bool ps2_write(ps2_device_t* dev, unsigned char command) {
    gpio_interrupts_disable();

    gpio_set_output(dev->clock);
    // request to send: clock low, data low, clock release
    gpio_write(dev->clock, 0); // 1
    timer_delay_us(200); // 1 clock low
    gpio_set_output(dev->data);
    gpio_write(dev->data, 0); // 2 data low
    gpio_write(dev->clock, 1); 
    gpio_set_input(dev->clock); // 3 release clock
    while (gpio_read(dev->clock) == 1) {} // 4 wait til Clock low

    unsigned int parity = 0;
    //wait_for_falling_clock_edge(dev);
    for (int i = 0; i < 8; i++) {
        unsigned int curBit = (command >> i) & 1;
        if (curBit) parity++;
        gpio_write(dev->data, curBit); // 5 least sign bit first
        wait_for_falling_clock_edge(dev); // 6, 7
    }    
    gpio_write(dev->data, (parity % 2) == 0); // parity bit
    wait_for_falling_clock_edge(dev); 
    gpio_write(dev->data, 1); // stop
    gpio_set_input(dev->data); // 9 release data
    wait_for_falling_clock_edge(dev);

    gpio_interrupts_enable();
    return true;
}

