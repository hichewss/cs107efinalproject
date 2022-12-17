#include "i2c.h"
#include "printf.h"
#include "nunchuck.h"
#include "timer.h"

//tested values that the nunchuck returns while idling, used to process raw data.
//ideally would be calculated by pulling calibration data from the nunchuck but the homebrew datasheets
//disagree on what register the data is stored at. Bummer
#define SX_DEFAULT 127;
#define SY_DEFAULT 128;

// the device address used to calibrate raw values read in
static const unsigned char dev = 0x52;
// the struct containing values the nunchuck calculates upon powerup
static calibration_values cvals;
// struct containing controller data that the raw info is sorted into
static nunchuck_event_t* nunchuck_vals;
// array containing raw controller data that is sorted into above struct
static char nunchuck_raws[6];

//some helper functions that are unused in the demo version. Nice to have.
void nunchuck_calibrate(void);
void nunchuck_cal_data_dump(char* cal_data);
void nunchuck_raw_data_dump(void);

void nunchuck_init(void)
{
	i2c_init(); 
	//decrypt the nunchuck by writing 0x55 and 0x00 to registers 0xF0 and 0xFB respectively
	char temp[2] = {0xF0, 0x55};
	i2c_write(dev, temp, 2);

	//give the nunchuck time to configure
	timer_delay_us(10);

	//write 0x00 to 0xFB
	temp[0] = 0xFB; 
	temp[1] = 0x00;
	i2c_write(dev, temp, 2);

	//give the nunchuck time
	timer_delay_us(10);

	//pull calibration data (does not quite work yet, will need further devling into documentation)
	nunchuck_calibrate(); 
	printf("nunchuck initialized\n");
}

void nunchuck_read(void)
{
	char temp[2] = {0x00, 0x00}; //load a dummy buffer
	i2c_write(dev, temp, 1); //write a zero to tell the nunchuck to give data from the 0 register
	i2c_read(dev, nunchuck_raws, 6); //controller data sent in six bytes
}	

nunchuck_event_t* nunchuck_read_event(void)
{
	nunchuck_read(); //load raw values
	nunchuck_vals->dx = (int)nunchuck_raws[0] - SX_DEFAULT; //pull stick xy from bytes 
	nunchuck_vals->dy = (int)nunchuck_raws[1] - SY_DEFAULT; //and center the raw values
	unsigned char temp = nunchuck_raws[5]; //button data is in last two bits of the sixth byte
	nunchuck_vals->BC = (bool)!(temp & 0b10); //flip the bits so that pressed is 1
	nunchuck_vals->BZ = (bool)!(temp & 0b1);
	return nunchuck_vals;
}

void nunchuck_calibrate(void)
{
	//buffer to hold initialization data for calibration
	
	char cal_data[16];

	//make a query to the nunchuck's 0x20 register where calibration data is held
	cal_data[0] = 0x20;
	i2c_write(dev, cal_data, 1);

	//then read the 16 bytes of calibration data
	i2c_read(dev, cal_data, 16);

	unsigned int nG_bits = (unsigned int)cal_data[3];
	unsigned int G_bits = (unsigned int)cal_data[7];
	unsigned int temp = 0;

	//pull bits 9-2 from first byte and 1-0 from third byte to get zero gravity x val
	temp = (unsigned int)cal_data[0];
	cvals.nG_Xval = temp << 2;
	cvals.nG_Xval |= (nG_bits >> 2) & 0b11;

	//same as above, for y
	temp = (unsigned int)cal_data[1];
	cvals.nG_Yval = temp << 2;
	cvals.nG_Yval |= (nG_bits >> 4) & 0b11;
	
	//now z
	temp = (unsigned int)cal_data[2];
	cvals.nG_Zval = temp << 2;
	cvals.nG_Zval |= (nG_bits >> 6) & 0b11;
	
	//pull bits 9-2 from first byte and 1-0 from seventh byte to get one gravity x val
	cvals.G_Xval = (unsigned int)cal_data[4];
	cvals.G_Xval = cvals.G_Xval << 2;
	cvals.G_Xval |= (G_bits >> 2) & 0b11;

	//same as above, for y
	cvals.G_Yval = (unsigned int)cal_data[5];
	cvals.G_Yval = cvals.G_Yval << 2;
	cvals.G_Yval |= (G_bits >> 4) & 0b11;
	
	//now z
	cvals.G_Zval = (unsigned int)cal_data[6];
	cvals.G_Zval = cvals.G_Yval << 2;
	cvals.G_Zval |= (G_bits >> 6) & 0b11;

	//pull byte data for the joystick values
	cvals.Xmax = cal_data[8]; //X joystick range and center values
	cvals.Xmin = cal_data[9];
	cvals.Xmid = cal_data[10];
	cvals.Ymax = cal_data[11]; //Y values now
	cvals.Ymin = cal_data[12];
	cvals.Ymid = cal_data[13];
//	nunchuck_cal_data_dump(cal_data);
}

void nunchuck_cal_data_dump(char* cal_data)
{
	printf("calibration raw data dump\n");
	for (int i = 0; i < 16; i++) {
		printf("char[%d] : %d\n", i, cal_data[i]);
	}
	
	printf("calibration struct data dump\n");
	printf("cvals.nG_Xval is %d\n", cvals.nG_Xval);
	printf("cvals.nG_Yval is %d\n", cvals.nG_Yval);
	printf("cvals.nG_Zval is %d\n", cvals.nG_Zval);
	printf("cvals.G_Xval is %d\n", cvals.G_Xval);
	printf("cvals.G_Yval is %d\n", cvals.G_Yval);
	printf("cvals.G_Zval is %d\n", cvals.G_Zval);
	printf("cvals.Xmax is %d\n", cvals.Xmax);
	printf("cvals.Xmin is %d\n", cvals.Xmin);
	printf("cvals.Xmid is %d\n", cvals.Xmid);
	printf("cvals.Ymax is %d\n", cvals.Ymax);
	printf("cvals.Ymin is %d\n", cvals.Ymin);
	printf("cvals.Ymid is %d\n", cvals.Ymid);
}

void nunchuck_raw_data_dump(void)
{
	printf("dx: %d\n", nunchuck_vals->dx);
	printf("dy: %d\n", nunchuck_vals->dy);
	printf("BC: %d\n", nunchuck_vals->BC);
	printf("BZ: %d\n\n", nunchuck_vals->BZ);
}
 
