#include <stdio.h>
#include <c6x.h>
#include <csl.h>
#include <csl_mcbsp.h>
#include <csl_irq.h>
#include "dsk6713.h"
#include "dsk6713_aic23.h"
#include "C6xdsk.h"



#define pi 3.14159265

#define FILTER 1

/// 2nd order
short a1[3]={16384,49941,7197};
short b1[3]={16384,56697,16383};
short a2[3]={16384,43532,15443};
short b2[3]={16384,43634,16384};
short g1=4268;
short g2=320;
short w1[3]={0};
short w2[3]={0};
short y1,y2;

///4th order
short w[5]={0};
short a[5]={8192,46736,21792,53352,3392};
short b[5]={9607 ,47510, 26142, 47510, 9607};
short g=14459;

short sine_in[512]={0};
short outs[512]={0};
float t;
int i=0;
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
	DSK6713_AIC23_setFreq(hCodec, DSK6713_AIC23_FREQ_48KHZ);
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
  short input=(data>>16);
	// "data" contains both audio channels

	// ---------------------- Useful code goes here ----------------------


	if (FILTER==1){
		 	 	 sine_in[i%512]=input;
				w1[0]=-((a1[1]*w1[1]*2)>>15)-((a1[2]*w1[2]*2)>>15)+((g1*input)>>15);
				y1=((b1[1]*w1[1]*2)>>15)+((b1[2]*w1[2]*2)>>15)+((b1[0]*2*w1[0])>>15);
									////// 2nd filter
				w2[0]=-((a2[1]*w2[1]*2)>>15)-((a2[2]*w2[2]*2)>>15)+((g2*y1)>>15);
				y2=((b2[1]*w2[1]*2)>>15)+((b2[2]*w2[2]*2)>>15)+((b2[0]*2*w2[0])>>15);
				outs[i%512]=y2;

				w1[2]=w1[1];
				w1[1]=w1[0];

				w2[2]=w2[1];
				w2[1]=w2[0];
				i++;
			}
	else if(FILTER==2){


		sine_in[i%512]=input;
				w[0]=((g*input)>>15)-((a[1]*w[1]*4)>>15)-((a[2]*w[2]*4)>>15)-((a[3]*w[3]*4)>>15)-((a[4]*w[4]*4)>>15);
				outs[i%512]=((b[0]*w[0])>>15)+((b[1]*w[1])>>15)+((b[2]*w[2])>>15)+((b[3]*w[3])>>15)+((b[4]*w[4])>>15);
				y2=outs[i%512];
				i++;
				int j;
				for(j=4;j>0;j--){
					w[j]=w[j-1];
				}
	// -------------------------------------------------------------------

	// process "data", or pass another variable to change the output
	output_leftright_sample(y2);
	}
}
