#include <stdio.h>
#include <c6x.h>
#include <csl.h>
#include <csl_mcbsp.h>
#include <csl_irq.h>
#include "dsk6713.h"
#include "dsk6713_aic23.h"
#include "C6xdsk.h"
#include <math.h>



#define N 205
#define pi 3.14159265
#define Fs 8000.0
#define FILTER 1
#define select 1
///               *2      *2     *2     *2     *2     *2
short coeff[8]={0x6D02,0x68AD,0x63FC,0x5EE7,0x4A70,0x4090,0x6521,0x479C};
//short coeff[8]={27906,26797 ,25596, 24295 , 19056 ,16528, 12944 , 9166};

float t;

short temp=0;
short g1=4268;
short g2=1320;
short Q[8][3]={0};
short x[N]={0};
short sine_in[512]={0};
int f_low[4]={697,770,852,941};
int f_high[4]={1209,1336,1477,1633};
int i=0;
int j,r,k;
short buff[N]={0};
short y[8]={0};
short max1=0;short max2=0;
int index1,index2,l;
short y1,y2,y3;


static DSK6713_AIC23_CodecHandle hCodec;							// Codec handle
static DSK6713_AIC23_Config config = { \
		0x0017,  /* 0 DSK6713_AIC23_LEFTINVOL  Left line input channel volume */ \
		0x0017,  /* 1 DSK6713_AIC23_RIGHTINVOL Right line input channel volume */\
		0x01f9,  /* 2 DSK6713_AIC23_LEFTHPVOL  Left channel headphone volume */  \
		0x01f9,  /* 3 DSK6713_AIC23_RIGHTHPVOL Right channel headphone volume */ \
		0x0011,  /* 4 DSK6713_AIC23_ANAPATH    Analog audio path control */      \
		0x0000,  /* 5 DSK6713_AIC23_DIGPATH    Digital audio path control */     \
		0x0000,  /* 6 DSK6713_AIC23_POWERDOWN  Power down control */             \
		0x0043,  /* 7 DSK6713_AIC23_DIGIF      Digital audio interface format */ \
		0x0001,  /* 8 DSK6713_AIC23_SAMPLERATE Sample rate control */            \
		0x0001   /* 9 DSK6713_AIC23_DIGACT     Digital interface activation */   \
	};  // Codec configuration with default settings

interrupt void serial_port_rcv_isr(void);
void init_hw_interrupts(void);

void main()
{

	DSK6713_init();		// Initialize the board support library, must be called first
	hCodec = DSK6713_AIC23_openCodec(0, &config);	// open codec and get handle
	// set codec sampling frequency 48kHz
	DSK6713_AIC23_setFreq(hCodec, DSK6713_AIC23_FREQ_8KHZ);
	*(unsigned volatile int *)McBSP1_RCR = 0x00A0;
	*(unsigned volatile int *)McBSP1_XCR = 0x00A0;
//	comm_intr();
	init_hw_interrupts();
	while(1);  // wait for interrupts

}

// interrupt service routine
void init_hw_interrupts(void)
{

	IRQ_globalDisable();			// Globally disables interrupts
	IRQ_nmiEnable();				// Enables the NMI interrupt

	// Maps an event to a physical interrupt
	// We can set the interrupt number wherever we like in vectors.asm ( this is the interrupt number )
	// Interrupt number is the second argument in IRQ_map define
	// The first argument must be set to a physical event and here we use the mcbsp1
	// and we want the receive interrupt so we have RINT1
	IRQ_map(IRQ_EVT_RINT1, 11);
	IRQ_enable(IRQ_EVT_RINT1);		// Enables the event
	IRQ_globalEnable();				// Globally enables interrupts

}

// interrupt service routine
interrupt void serial_port_rcv_isr()
{

	int data = input_leftright_sample();
	// "data" contains both audio channels

	// ---------------------- Useful code goes here ----------------------
	short left=(short)(data>>16);
	sine_in[i%512]=left;



	for(j=0;j<8;j++){
		if(j<6){
			 y1=left;
			 y2=(short)((coeff[j]*Q[j][1])>>14);
			  y3=Q[j][0];
			  Q[j][2]=y1+y2-y3;
		}
		else{
            y1=left;
            y2=(short)((coeff[j]*Q[j][1])>>15);
            y3=Q[j][0];
            Q[j][2]=y1+y2-y3;
		}

		Q[j][0]=Q[j][1];
		Q[j][1]=Q[j][2];
	}

		if(i==N-1){
			i=0;
			for(j=0;j<8;j++){
				if(j<6){
					y1=(short)((Q[j][2]*Q[j][2])>>15)+(short)((Q[j][0]*Q[j][0])>>15);
					y2=(short)((coeff[j]*Q[j][2])>>14);
					y3=(short)((y2*Q[j][0])>>15);
					y[j]=(y1+y3);
				}
				else{
					y1=(short)((Q[j][2]*Q[j][2])>>15)+(short)((Q[j][0]*Q[j][0])>>15);
					y2=(short)((coeff[j]*Q[j][2])>>15);
					y3=(short)((y2*Q[j][0])>>15);
					y[j]=(y1+y3);
				}
				Q[j][0]=0;
				Q[j][1]=0;
				Q[j][2]=0;
			}

			max1=y[0];
			max2=y[4];
			for(l=0;l<4;l++){
				if(y[l]>max1){
					max1=y[l];
					index1=l;
				}
				}

			printf("f low is %d  \n",f_low[index1]);


			for(l=4;l<8;l++){
				if(y[l]>max2){
					max2=y[l];
					index2=l-4;
					}
				}

			printf("f high is %d  \n",f_high[index2]);


				max1=0;max2=0;

		}

	// -------------------------------------------------------------------
		i++;
	// process "data", or pass another variable to change the output
	output_leftright_sample(data);

}




