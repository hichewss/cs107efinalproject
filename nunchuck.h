#ifndef NUNCHUCK_H
#define NUNCHUCK_H

#include "gpio.h"
#include "stdbool.h"
/* 
 * Module to read inputs from the Nintendo Nunchuck
 *
 * Implemented for Cs107e Project Fall 2022
 *
 * Author: Vincent Thai
 *
 * Last updated: Dec 2022
 */

//struct to hold data used by the project game; Accelerometer data not currently incl9uded
typedef struct {
	int dx, dy;
	bool BC, BZ;
} nunchuck_event_t;

//this struct contains calibration data calculated by the nunchuck upon powerup. Used to
//determine what values the nunchuck sends when idle
typedef struct {
	unsigned int nG_Xval;
	unsigned int nG_Yval;
	unsigned int nG_Zval;
	unsigned int G_Xval;
	unsigned int G_Yval;
	unsigned int G_Zval;
	unsigned char Xmax;
	unsigned char Xmin;
	unsigned char Xmid;
	unsigned char Ymax;
	unsigned char Ymin;
	unsigned char Ymid;
} calibration_values;

/*
 * 'nunchuck_init': Required initialization for the Nunchuck controller.
 *
 * The nunchuck must be initialized before any values can be read.
 *
 * This function creates an I2C device struct with clock and data line specified in the i2c
 * library file, and sets fixed address 0x52 for the nunchuck.
 * Initializes the nunchuck and decrypts the data by writing 0x55 to register at 0xF0
 * and then 0x00 to 0xFB.
 * Also fills a struct with the values the nunchuck would send if idling (no user input/interaction)
 * by continuously reading 16 bytes from register 0x20 of the nunchuck.
 *
 */
void nunchuck_init(void);

/*
 * 'nunchuck_read_event': Returns a struct of controller data used for the aim game.
 *
 * Requires previous init.
 *
 * This function relies on a few helpers that call i2c write and i2c read to pull 6 bytes of data from 
 * nunchuck register 0x00. The button/joystick/accelerometer data is extracted from theses bytes. The
 * data is then zeroed/calibrated and put into the returned struct.
 *
 * Acceleration data not yet implemented (not relevant for demo day game progra,,)
 *
 */
nunchuck_event_t* nunchuck_read_event(void);
#endif
