#include "sensitivity.h"

//absolute value helper function
int abs(int n)
{
	if (n > 0) {
		return n;
	} else {
		return -1 * n;
	}
}

//base 2 logarithm helper function
int log2(int n)
{
	if (!n) {
		return n;
	}
	int result = -1;
	int temp = abs(n);
	while (temp) {
		result++;
		temp = temp >> 1;
	}
	if (n < 0) {
		return (-1 * result);
	} else {
		return result;
	}
}

//general form logarithm helper
int logb(int input, int base)
{
	    return log2(input) / log2(base);
}

//input flattening function
int cap(int n, int cap) 
{
	if (abs(n) > cap) {
		if (n < 0) {
			return -1 * cap;
		} else {
			return cap;
		}
	} else {
	       return n;
	}	       
}


