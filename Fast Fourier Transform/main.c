#include <stdio.h>
#include <c6x.h>

#include <csl.h>
#include <csl_mcbsp.h>
#include <csl_irq.h>
#include "dsk6713.h"
#include "dsk6713_aic23.h"
#include "C6xdsk.h"
#include <math.h>
#include "ham64.h"

#define nx 64
#define pi 3.14159265
#define Fs 48000.0
#define F  10000.0
#define NORM 0.01

void fft_calc();
void fft_buffers();
int flag=0;
int i,j,counter;
short index[8], x[2*nx],w[nx],input[nx];
int x1[nx], x_fft[nx];
short input_buff[2*nx],fft_in[2*nx],in[nx];
int fft_out[nx],out_buff[nx];

static DSK6713_AIC23_CodecHandle hCodec;                            // Codec handle
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



    for (i = 0; i < nx/2; i++){
            w[i*2] = 32767 * -cos( i*2*pi/nx );
            w[i*2 + 1] = 32767 * -sin( i*2*pi/nx );
        }
    bitrev_index(index,nx);




    counter=0;
/*
    for(i=0;i<nx;i++){
        x[2*i]=(short)(32767*NORM*sin(i*2*pi*F/Fs));
        x1[i]=(short)(32767*NORM*sin(i*2*pi*F/Fs));
    }


    fft_calc();
    while(1);
*/
    flag=0;
    DSK6713_init();       // Initialize the board support library, must be called first
    hCodec = DSK6713_AIC23_openCodec(0, &config); // open codec and get handle
    // set codec sampling frequency 48kHz
    DSK6713_AIC23_setFreq(hCodec, DSK6713_AIC23_FREQ_16KHZ);
    *(unsigned volatile int *)McBSP1_RCR = 0x00A0;
    *(unsigned volatile int *)McBSP1_XCR = 0x00A0;
     // comm_intr();
    init_hw_interrupts();

    while(1){
    	if(flag){
    		for(i=0;i<nx;i++){
    			fft_in[2*i]=input_buff[2*i];
    			fft_in[2*i+1]=0;
    		}
    		fft_calc();
    		counter=counter;

    		for(i=0;i<nx;i++){
				out_buff[i]=fft_out[i];
   
    		}
    	}

    }

}


// interrupt service routine
void init_hw_interrupts(void)
{

    IRQ_globalDisable();            // Globally disables interrupts
    IRQ_nmiEnable();                // Enables the NMI interrupt

    // Maps an event to a physical interrupt
    // We can set the interrupt number wherever we like in vectors.asm ( this is the interrupt number )
    // Interrupt number is the second argument in IRQ_map define
    // The first argument must be set to a physical event and here we use the mcbsp1
    // and we want the receive interrupt so we have RINT1
    IRQ_map(IRQ_EVT_RINT1, 11);
    IRQ_enable(IRQ_EVT_RINT1);      // Enables the event
    IRQ_globalEnable();             // Globally enables interrupts

}

// interrupt service routine
interrupt void serial_port_rcv_isr()
{

	int data=input_leftright_sample();
	in[counter]=(data>>16)*ham64[counter];
	input_buff[2*counter]=(short)(((data>>16)*ham64[counter])>>15);
	input_buff[2*counter+1]=0;

	if(counter==(nx-1)){

		flag=1;

		counter=0;
	}
	else{
		flag=0;
		counter++;
	}

	output_leftright_sample(out_buff[counter]);


}
void fft_calc(){


    DSP_radix2(nx,&fft_in,w);

    DSP_bitrev_cplx(&fft_in, index,nx) ;
    for (i = 0; i < nx; i++){
    //  x_fft[i]=(x1[i]& 0xFFFF0000)*(x1[i]& 0xFFFF0000)+(x1[i]& 0x0000FFFF)*(x1[i]& 0x0000FFFF);
    	fft_out[i]=(((int)fft_in[2*i]*(int)fft_in[2*i]+(int)fft_in[2*i+1]*(int)fft_in[2*i+1]));

    }

}



