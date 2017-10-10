#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <c6x.h>
#include <csl.h>
#include <csl_mcbsp.h>
#include <csl_irq.h>
#include "dsk6713.h"
#include "dsk6713_aic23.h"
#include "C6xdsk.h"
#include <math.h>
//#include "sine8000_table.h"


#define F 8000.0

#define f1 3500.0
#define f2 520.0
#define Fs 48000.0
#define N  256
#define D 32
#define beta 2.0
#define M 30 // number of parameters
#define alpha 0.00001
#define pi 3.14159265
#define FSize 235000

float freq[3]={310.0,980.0,1123.0};
float weight[M]={0.0};
float x[M];
float error,desired,xn2,y;
int i,j,wr=0,re=0,k;
float timer=0;
float noise;

float input[N],err[N],output[N];
float w1[N],w2[N],d[N];
short voice[N];
float sum=0;
float delay[D]={0.0};

void ask4_2();
void ask4_3();
void ask4_4();
void nlms(float desired,float in,int rep);



static DSK6713_AIC23_CodecHandle hCodec;	// Codec handle
static DSK6713_AIC23_Config config = { \
		0x0017,  /* 0 DSK6713_AIC23_LEFTINVOL  Left line input channel volume */ \
		0x0017,  /* 1 DSK6713_AIC23_RIGHTINVOL Right line input channel volume */\
		0x01f9,  /* 2 DSK6713_AIC23_LEFTHPVOL  Left channel headphone volume */  \
		0x01f9,  /* 3 DSK6713_AIC23_RIGHTHPVOL Right channel headphone volume */ \
		0x0015,  /* 4 DSK6713_AIC23_ANAPATH    Analog audio path control */      \
		0x0000,  /* 5 DSK6713_AIC23_DIGPATH    Digital audio path control */     \
		0x0000,  /* 6 DSK6713_AIC23_POWERDOWN  Power down control */             \
		0x0043,  /* 7 DSK6713_AIC23_DIGIF      Digital audio interface format */ \
		0x0001,  /* 8 DSK6713_AIC23_SAMPLERATE Sample rate control */            \
		0x0001   /* 9 DSK6713_AIC23_DIGACT     Digital interface activation */   \
	};  // Codec configuration with default settings

interrupt void serial_port_rcv_isr(void);
void init_hw_interrupts(void);


void nlms(float desired,float in,int rep){

	//rep is current repetition
    y=0;
    re=rep+1;
    for(k=M-1;k>=0;k--){

        y+=(weight[k]*x[re%M]);

        re++;
    }

   error=(desired-y);
   err[rep]=error;
   output[rep]=in-y;
    if(fabs(error)<0.0001){
        error=0.0;
    }
    for(k=M-1;k>=0;k--){

            xn2+=(x[k]*x[k]);//metro x^2
        }
    re=rep+1;
    for(k=M-1;k>=0;k--){
        weight[k]=weight[k]+(beta/(alpha+xn2))*error*x[re%M];
        re++;
    }
}



void ask4_2(){

	for(i=0;i<N;i++){
		w1[i]=sin(2*pi*(f2/Fs)*i);
		w2[i]=cos(2*pi*(f2/Fs)*i);
		d[i]=sin(2*pi*(f1/Fs)*i);
		input[i]=d[i]+w1[i];

	}

	for(i=0;i<N;i++){


		x[i%M]=w2[i];
		nlms(w1[i%N],input[i],i);


	}

}

void ask4_3(){


	 int data = input_leftright_sample();
	 noise=0.01*sin(2*pi*(freq[((i/48000)%48000)%3]/Fs)*i);
	 input[i%N]=(float)((data>>16))/32768.0+noise;
	 delay[(i+10)%D]=input[i%N];
	 x[i%M]=delay[i%D];
	 nlms(noise,input[i%N],i%N);
	 output_leftright_sample((short)(32767*output[i%N]));
	 i++;

}


void ask4_4(){

	 noise=0.1*cos(2*pi*(f2/Fs)*i);
		 output_left_sample((short)(32768*noise));

		 int data = input_leftright_sample();
		  x[i%M]=noise;
		  input[i%N]=(float)((data>>16)/32768.0);//input=error
		  nlms(noise,input[i%N],i%N);
		  output_right_sample((short)(32768*y));//(short)(32767*y));
		  i++;




}



void main()
{

	//ask4_2();




	i=0;


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

	//ask4_3();
	//ask4_4();




}



