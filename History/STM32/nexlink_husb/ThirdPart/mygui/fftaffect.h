#ifndef __OLED_FFT_H
#define __OLED_FFT_H

#include "main.h"
#include "nv3030b.h"
#include "stdlib.h"
#include "arm_math.h"

#define FFT_SAMPLE 256

#define TrumHeight   LCD_W
#define TrumWidth    1
#define TrumInterval 1
#define TrumNum	   	 256
	
void FFT_Run(int32_t* fftraw);
void Display_Style1(float32_t* freqs);
void Display_Style2(void);
void Display_Style3(void);
void Display_Style4(void);
void Display_Style5(void);
void Display_Style6(void);


#endif
