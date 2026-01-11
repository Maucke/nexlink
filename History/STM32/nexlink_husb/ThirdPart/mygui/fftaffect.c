#include "fftaffect.h"
#include "main.h"
#include "gpio.h"
#include "math.h"
#include "arm_math.h"

//static int16_t dampfft[FFT_SAMPLE];

//static int absl(int num)
//{
//	if(num>=0)
//		return num;
//	else
//		return -num;
//}

//void FFT_Run(int32_t* fftraw)
//{
//	int i;
//	float step;
//	for(i=0;i<FFT_SAMPLE;i++)
//	{
//		step = absl(fftraw[i]-dampfft[i])/8+1;
//		if(fftraw[i] > dampfft[i]+step)
//			dampfft[i] += step;
//		else if(fftraw[i] < dampfft[i]-step)
//			dampfft[i] -= step;
//		else if(absl(fftraw[i]-dampfft[i])<=step)
//			dampfft[i] = fftraw[i];
//	}
//}
//int random_value;extern uint16_t chosen_freqs[32];
//extern uint16_t maxValue;
//extern uint32_t maxIndex;

//float32_t maxMagnitude = 32;
//uint8_t audio_bar_height[32]; // sizes for the individual bars
//uint8_t audio_bar_peak[32]; // positions for the individual peaks (lines over the bars)
//float value = 2.5;
//void Display_Style1(float32_t* freqs)
//{
//	for (int i=0; i<32; i++) // loop for every fraquency (63Hz, 160Hz, 400Hz, 1kHz, 2.5kHz, 6.25kHz and 16kHz)
//	{

//		//random_value = freqs[chosen_freqs[i]]/maxMagnitude;//freqs[chosen_freqs[i]];//(rand()%53 + 0);//bandValues[i];//(rand()%53 + 0);; // calculate random value between 0-1024

//		random_value = freqs [chosen_freqs[i]]-maxValue-maxMagnitude;//maxValue*2;//bandValues[i]/maxMagnitude;
//		if (random_value > 40) random_value = 40;
//		audio_bar_height[i] = audio_bar_height[i] + ((random_value - audio_bar_height[i]))/value; // update the bar with a new value (slowly)

//		// calculate the peak position
//		if (audio_bar_peak[i] < audio_bar_height[i]) // if the peak is below the current bar size
//		{
//			audio_bar_peak[i] = audio_bar_height[i]; // move peak to the new max. position (i.e. size of the bar)
//		}
//		else if (audio_bar_peak[i] > audio_bar_height[i])
//		{ // if the bar is lower than the peak
//			audio_bar_peak[i]--; // slowly move the peak down, one pixel every frame
//			//audio_bar_peak[i]--; // slowly move the peak down, one pixel every frame
//			//audio_bar_peak[i]--; // slowly move the peak down, one pixel every frame
//		}


//		NV3030B_DrawBox(1 + i*5, 40, 3, audio_bar_height[i], 0x3fe0);
//		NV3030B_DrawBox(1 + i*5, 40-audio_bar_height[i], 3, audio_bar_height[i], 0xf7e0);
//		NV3030B_DrawBox(1 + i*5, 40-audio_bar_peak[i], 3, 3, 0x04bf);//(rand()%65534 + 0)); // draw peak 64-audio_bar_peak[i]
//		NV3030B_DrawBox(1 + i*5, 40+audio_bar_peak[i], 3, 3, 0xf816);//(rand()%65534 + 0)); // draw peak 64-audio_bar_peak[i]
//	}
//}

//void Display_Style2(void)
//{
//	uint16_t i = 0;
//	uint16_t Index;
//	for(i = 0; i < SCR_WIDTH/3; i++)	
//	{
//		Index = (float)i*FFT_SAMPLE/SCR_WIDTH*3+1;
//		flow_pot[i] = SCR_HEIGHT*2/3 - (float)(dampfft[Index]/3);
//		if(flow_pot[i] < 2)
//			flow_pot[i] = 2;
//		
//		if(fall_pot[i]+3 > flow_pot[i]) 
//			fall_pot[i] = flow_pot[i]-3;
//		else if(fall_pot[i]+3 < flow_pot[i]) 
//			fall_pot[i] ++;
//		oled.Draw_Line(3*i,flow_pot[i]+1,3*i,SCR_HEIGHT*2/3);
//		oled.Draw_Line(3*i+1,flow_pot[i]+1,3*i+1,SCR_HEIGHT*2/3);
//		
//		oled.Draw_Line(3*i,fall_pot[i]+1+1,3*i,fall_pot[i]+1+1,color_half);
//		oled.Draw_Line(3*i+1,fall_pot[i]+1+1,3*i+1,fall_pot[i]+1+1,color_half);
//		
//		oled.Draw_Line(3*i,(SCR_HEIGHT*2/3-flow_pot[i])/3+SCR_HEIGHT*2/3-1,3*i,SCR_HEIGHT*2/3-1,color_half);
//		oled.Draw_Line(3*i+1,(SCR_HEIGHT*2/3-flow_pot[i])/3+SCR_HEIGHT*2/3-1,3*i+1,SCR_HEIGHT*2/3-1,color_half);
//		
//	}
//	oled.Draw_Line(0,SCR_HEIGHT*2/3-1,SCR_WIDTH,SCR_HEIGHT*2/3-1,color_min);
//}


//void Display_Style3(void)
//{
//	uint16_t i = 0;
//	uint16_t j = 0;
//	uint16_t Index;
//	static u8 BackFlag[100]={0};
//	for(i = 0; i < SCR_WIDTH/4; i++)	
//	{
//		Index = (float)i*FFT_SAMPLE/SCR_WIDTH*4+1;
//		flow_pot[i] = TrumHeight - (u16)(dampfft[Index]/2);
//		if(flow_pot[i] < 4)
//			flow_pot[i] = 4;
//		
//		if(fall_pot[i]+3 > flow_pot[i]) 
//		{
//			fall_pot[i] = flow_pot[i]-3;
//			BackFlag[i] = 0;
//		}
//		else if(fall_pot[i]+3 < flow_pot[i]) 
//		{
//			if(!BackFlag[i])
//			{
//				if(fall_pot[i]>=4)
//					fall_pot[i] -= 3;
//				else if(fall_pot[i])
//					BackFlag[i] = 1;
//				else
//					fall_pot[i] += 2;
//			}
//			else
//				fall_pot[i] += 2;
//		}
////			HAL_GPIO_TogglePin(GPIOC, SYS_LED_Pin);
//			
//		oled.Draw_Line(4*i+0,fall_pot[i],4*i+0,fall_pot[i],color_half);
//		oled.Draw_Line(4*i+1,fall_pot[i],4*i+1,fall_pot[i],color_half);
//		oled.Draw_Line(4*i+2,fall_pot[i],4*i+2,fall_pot[i],color_half);
//		
//		oled.Draw_Line(4*i+0,flow_pot[i],4*i+0,TrumHeight-1);
//		oled.Draw_Line(4*i+1,flow_pot[i],4*i+1,TrumHeight-1);
//		oled.Draw_Line(4*i+2,flow_pot[i],4*i+2,TrumHeight-1);	
//		for(j=TrumHeight;j>=flow_pot[i];j-=2)
//			oled.Draw_Line(4*i+0,j,4*i+2,j,0);
//	}
//}

//int16 SampPoint[4][192*2];

//void Display_Style4(void)
//{

//	uint16_t i = 0;
//	uint16_t Index;
//	for(i = 0; i < SCR_WIDTH/3; i++)	
//	{
//		Index = (float)i*FFT_SAMPLE/SCR_WIDTH*3+1;
//		flow_pot[i] = TrumHeight - (u16)(dampfft[Index]/2);
//		if(flow_pot[i] < 2)
//			flow_pot[i] = 2;
//		SampPoint[0][i*2] = 3*i;
//		SampPoint[0][i*2+1] = flow_pot[i];
//		oled.Draw_Line(3*i,flow_pot[i],3*i,TrumHeight-1,color_half);
//	}
//	oled.Draw_LineS(SampPoint[0],SCR_WIDTH/3);
//}

//void Display_Style5(void)
//{

//	uint16_t i = 0;
//	uint16_t Index;
//	for(i = 0; i < SCR_WIDTH/3; i++)	
//	{
//		Index = (float)i*FFT_SAMPLE/SCR_WIDTH*3+1;
//		flow_pot[i] = SCR_HEIGHT/2 - (u16)(dampfft[Index]/4);
//		if(flow_pot[i] < 2)
//			flow_pot[i] = 2;
//		
//		SampPoint[0][i*2] = 3*i;
//		SampPoint[0][i*2+1] = flow_pot[i];
//		SampPoint[1][i*2] = 3*i;
//		SampPoint[1][i*2+1] = - flow_pot[i] + SCR_HEIGHT+1;
//		
//		oled.Draw_Line(3*i,SampPoint[0][i*2+1],3*i,SampPoint[1][i*2+1]-1,color_half);
//		
//	}
//	oled.Draw_LineS(SampPoint[0],SCR_WIDTH/3);
//	oled.Draw_LineS(SampPoint[1],SCR_WIDTH/3);
//}
//	
//void Display_Style6(void)
//{	
////	uint16_t i = 0; 
//	uint16_t i = 0;
//	uint16_t Index;
//	int Temp;
//	u8 MaxType = 0xff;
//	static int runCount = 0;
//	static float Rr,Rn,step;
//	static int runCount1 = 0;
//	static int runCount2 = 0;
////	if(Device_Msg.leftvol)
//		Rn = 50+Device_Msg.leftvol/(20*256);
////	else
////		Rn = 5;
//	
//	step = absl(Rn-Rr)/10+1;
//	
//	if(Rn > Rr+step)
//	{
//		Rr += step;
//	}
//	else if(Rn < Rr-step)
//	{
//		Rr -= step;
//	}
//	else if(absl(Rn-Rr)<=step)
//	{
//		Rr = Rn;
//	}
//	
//	runCount2++;
//	for(i = 0; i < 60; i++)	
//	{
//		Index = (runCount2+i*180/60)%60*3;
//				
//		if(fftraw[Index]<100)
//			Temp = Rr/2 - (u16)(fftraw[Index]/32);
//		else if(fftraw[Index]>200)
//			Temp = Rr/2 - (u16)(fftraw[Index]/24);
//		else
//			Temp = Rr/2 - (u16)(fftraw[Index]/8);
//		if(Temp < 2)
//			Temp = 0;
//			
//		if(flow_pot[i]+10 < Temp)
//			flow_pot[i] += 4;
//		else if(flow_pot[i]+3 < Temp)
//			flow_pot[i] += 2;
//		else if(flow_pot[i] < Temp)
//			flow_pot[i] += 1;
//		if(flow_pot[i] > (Temp+10))
//			flow_pot[i] -= 4;
//		else if(flow_pot[i] > (Temp+3))
//			flow_pot[i] -= 2;
//		else if(flow_pot[i] > Temp)
//			flow_pot[i] -= 1;
//		if(flow_pot[i]<MaxType)
//			MaxType = flow_pot[i];
//		if(flow_pot[i]<Rr)
//		{
//			SampPoint[0][i*2] = OCX+((Rr+flow_pot[i]-Rr/2)*cos(i*6*PI/180));
//			SampPoint[0][i*2+1] = OCY+((Rr+flow_pot[i]-Rr/2)*sin(i*6*PI/180));
//		}
//		else
//		{
//			SampPoint[0][i*2] = OCX+((Rr+Rr-flow_pot[i]+Rr-Rr/2)*cos(i*6*PI/180));
//			SampPoint[0][i*2+1] = OCY+((Rr+Rr-flow_pot[i]+Rr-Rr/2)*sin(i*6*PI/180));
////			SampPoint[0][i*2] = OCX;
////			SampPoint[0][i*2+1] = OCY;
//		}
//		SampPoint[1][i*2] = OCX+((Rr-flow_pot[i]+Rr/2)*cos(i*6*PI/180));
//		SampPoint[1][i*2+1] = OCY+((Rr-flow_pot[i]+Rr/2)*sin(i*6*PI/180));
//		
//		oled.Draw_Line(SampPoint[0][i*2],SampPoint[0][i*2+1],SampPoint[1][i*2],SampPoint[1][i*2+1],0xffff);
//		
//	}
//	for(i = 0; i < 60; i++)	
//	{
//		SampPoint[1][i*2] = OCX+((Rr+3-flow_pot[i]+Rr/2)*cos(i*6*PI/180));
//		SampPoint[1][i*2+1] = OCY+((Rr+3-flow_pot[i]+Rr/2)*sin(i*6*PI/180));
//	}
//	oled.Draw_Line(SampPoint[1][0*2],SampPoint[1][0*2+1],SampPoint[1][59*2],SampPoint[1][59*2+1]);
//	oled.Draw_LineS(SampPoint[1],60);
//	
//	for(i = 0; i < 60; i++)	
//	{
//		SampPoint[1][i*2] = OCX+((Rr+5-flow_pot[i]+Rr/2)*cos(i*6*PI/180));
//		SampPoint[1][i*2+1] = OCY+((Rr+5-flow_pot[i]+Rr/2)*sin(i*6*PI/180));
//	}
//	oled.Draw_Line(SampPoint[1][0*2],SampPoint[1][0*2+1],SampPoint[1][59*2],SampPoint[1][59*2+1],color_half);
//	oled.Draw_LineS(SampPoint[1],60,color_half);
//	
//	runCount++;
//	if(runCount>=20)
//		runCount = 0;
//	runCount1-=2;
//	if(runCount1<=0)
//		runCount1 = 20;
//	
////	HAL_Delay(2000);
////	oled.Draw_Circle(OCX,OCY,40-MaxType+20+2,color_half);
////	oled.Draw_Circle(OCX,OCY,40-MaxType+20+3,color_min);
//	Temp = 70-MaxType+20+3;
//	
//	oled.Draw_Line(OCX+(Temp*cos((runCount1+0)*6*PI/180)),OCY+(Temp*sin((runCount1+0)*6*PI/180)),OCX+(Temp*cos((runCount1+5)*6*PI/180)),OCY+(Temp*sin((runCount1+5)*6*PI/180)),color_half);
//	oled.Draw_Line(OCX+(Temp*cos((runCount1+20)*6*PI/180)),OCY+(Temp*sin((runCount1+20)*6*PI/180)),OCX+(Temp*cos((runCount1+25)*6*PI/180)),OCY+(Temp*sin((runCount1+25)*6*PI/180)),color_half);
//	oled.Draw_Line(OCX+(Temp*cos((runCount1+40)*6*PI/180)),OCY+(Temp*sin((runCount1+40)*6*PI/180)),OCX+(Temp*cos((runCount1+45)*6*PI/180)),OCY+(Temp*sin((runCount1+45)*6*PI/180)),color_half);
//	Temp = 40+MaxType-20-2;
////	oled.Fill_Circle(OCX,OCY,40+MaxType-20-4);
//	oled.Draw_Triangle(OCX+(Temp*cos(runCount*6*PI/180)),OCY+(Temp*sin(runCount*6*PI/180)),OCX+(Temp*cos((runCount+20)*6*PI/180)),OCY+(Temp*sin((runCount+20)*6*PI/180)),OCX+(Temp*cos((runCount+40)*6*PI/180)),OCY+(Temp*sin((runCount+40)*6*PI/180)),color_half);
//	Temp-=4;
//	oled.Draw_Triangle(OCX+(Temp*cos(runCount*6*PI/180)),OCY+(Temp*sin(runCount*6*PI/180)),OCX+(Temp*cos((runCount+20)*6*PI/180)),OCY+(Temp*sin((runCount+20)*6*PI/180)),OCX+(Temp*cos((runCount+40)*6*PI/180)),OCY+(Temp*sin((runCount+40)*6*PI/180)),color_min);
////HAL_Delay(100);
//}



