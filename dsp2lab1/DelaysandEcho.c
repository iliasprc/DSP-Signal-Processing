/*****************************************************************************
* FILENAME
*   Delays and echo.c
*
* DESCRIPTION
*   Delays and Echo using TMS320C6713 DSK Board and AIC23 audio codec.
*   Based on sampling rate of 48000 samples per second. Delay up to 4 seconds.  */
/*********************************************************************************/

#include <stdio.h>
#include "bargraph.h"
#include "stereo.h"
#include "switches.h"
#include "dsk6713.h"
#include "dsk6713_aic23.h"
#include "stdlib.h"
#include "math.h"
#include "c6713dsk.h"
#define N (48000 * 4)


int write;
int read;

// Codec configuration settings
DSK6713_AIC23_Config config = { \
	0x0017,  /* 0 DSK6713_AIC23_LEFTINVOL  Left line input channel volume */ \
	0x0017,  /* 1 DSK6713_AIC23_RIGHTINVOL Right line input channel volume */\
	0x01f9,  /* 2 DSK6713_AIC23_LEFTHPVOL  Left channel headphone volume */  \
	0x01f9,  /* 3 DSK6713_AIC23_RIGHTHPVOL Right channel headphone volume */ \
	0x0015,  /* 4 DSK6713_AIC23_ANAPATH    Analog audio path control */      \
	0x0000,  /* 5 DSK6713_AIC23_DIGPATH    Digital audio path control */     \
	0x0000,  /* 6 DSK6713_AIC23_POWERDOWN  Power down control */             \
	0x0043,  /* 7 DSK6713_AIC23_DIGIF      Digital audio interface format */ \
	0x0081,  /* 8 DSK6713_AIC23_SAMPLERATE Sample rate control */            \
	0x0001   /* 9 DSK6713_AIC23_DIGACT     Digital interface activation */   \
};


Uint16 delay_array[N]; /* Buffer for maximum delay of 4 seconds */

/*
 Uses DIP switches to control the delay time between 0 ms and
 4 seconds. 48000 samples represent 1 second.
*/

int get_delay_time(){
	return 48000*(user_switches_read()*(4.0/7.0));

}

/*
Take oldest sample from the array and replace with the newest
Uses a circular buffer because a straight buffer would be too slow.
*/

Uint32 delayed_input(Uint32 *val,int delay){

	if(read==N)read=0;
	if(write<N){
		delay_array[write]=*val;
		write++;
		read++;
	}
	else{
		write=0;
		delay_array[write]=*val;
		write++;
		read++;
	}
	Uint32 left ;
	Uint16 right ;
	right=delay_array[read];
	left=*val<<16;
	Uint32 out=left|right;
return out;
}




/*
Fill delay array with zeroes to prevent noise / clicks.
*/

delay_array_clear(){
	int i;
	for(i=0;i<N;i++){
		delay_array[i]=0;
	}
}

/*
Show status on Stdout window.
*/

switch_status_display(){

	int sw3=IO_PORT&(0x00000080);
	sw3=sw3>>7;
	int sw2=IO_PORT&(0x00000040);
	sw2=sw2>>6;
	int sw1=IO_PORT&(0x00000020);
	sw1=sw1>>5;
	int sw0=IO_PORT&(0x00000010);
	sw0=sw0>>4;

	printf("status of switch 3 %x",sw3);
	printf("status of switch 2 %x",sw2);
	printf("status of switch 1 %x",sw1);
	printf("status of switch 0 %x",sw0);

}


int main(void)
{


	DSK6713_AIC23_CodecHandle hCodec;

	// Initialize BSL
	DSK6713_init();

	//Start codec
	hCodec = DSK6713_AIC23_openCodec(0, &config);

	// Set  frequency to 48KHz
	DSK6713_AIC23_setFreq(hCodec, DSK6713_AIC23_FREQ_48KHZ);

//------------------------------------------------------------------------------------------
//                            Enter the appropriate code here
//------------------------------------------------------------------------------------------
	*(unsigned volatile int *) McBSP1_RCR = 0x000000A0;
	*(unsigned volatile int *) McBSP1_XCR = 0x000000A0;
	delay_array_clear();
	int delay,temp;

	delay=get_delay_time();
	write=delay;
	read=0;
	Uint32 val,out;
	while(1){
		while(!DSK6713_AIC23_read(hCodec, &val )){

		}
		temp=get_delay_time();
		if(temp!=delay){
			write=delay;
			read=0;
			delay=temp;
		}
		switch_status_display();
		out=delayed_input(&val, delay);
		while(!DSK6713_AIC23_write(hCodec,out)){

		}

	}

//------------------------------------------------------------------------------------------
	return (0);
}
