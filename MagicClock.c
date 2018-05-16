/* Include Files */
#include <stdio.h>
#include "platform.h"
#include "xparameters.h"
#include "xgpio.h"
#include "xstatus.h"
#include "xuartps.h"
#include "xil_printf.h"
#include "VGA.h"
#include "xil_mmu.h"
#include "xil_assert.h"

/* Definitions */
//#define UART_DEVICE_ID  XPAR_XUARTPS_0_DEVICE_ID
//Make sure these are between 0 - 255 each
#define RBG_background_default 25
#define RBG_seconds_default 50
#define RBG_minutes_default 100
#define RBG_hours_default 90
#define RGB_COLON_default 28

// Clock Dimensions Characters:

/* Ideal Layout :
 * Y broken into 3 160 pixels rectangles
 * X broken into 10 64 pixels rectangles
 * Each X rectangle holds one character (8 of them). 2 rectangles are for space to fram
 *
 *
 *
 * 160 Y LONG _BLANK
 * BLANK    H1    H0    COLON    M1    M0    COLON    S1    S0        BREAK
 * 160 Y LONG Y_BLANK
 *
 *
 *
 *
 *
 */
#define CharSECONDS_X1_MIN  64*7
#define CharSECONDS_X1_MAX  64*8 - 10
#define CharSECONDS_X_MIN   64*8
#define CharSECONDS_X_MAX   64*9 -10
#define CharSECONDS_Y_MIN  160
#define CharSECONDS_Y_MAX  320

#define CharMIN_X1_MIN   64*4
#define CharMIN_X1_MAX   64*5-10
#define CharMIN_X_MIN    64*5
#define CharMIN_X_MAX    64*6-10
#define CharMIN_Y_MIN    160
#define CharMIN_Y_MAX    320

#define CharHOURS_X1_MIN  64*1
#define CharHOURS_X1_MAX  64*2 - 10
#define CharHOURS_X_MIN   64*2
#define CharHOURS_X_MAX   64*3 - 10
#define CharHOURS_Y_MIN   160
#define CharHOURS_Y_MAX   320

#define COLON_Y_MIN  160
#define COLON_Y_MAX  320
#define COLONLeft_X_MIN 64*3
#define COLONLeft_X_MAX 64*4 - 10
#define COLONRight_X_MIN 64*6
#define COLONRight_X_MAX 64*7-10


//hours , minutes, seconds from HW
/* Definitions */
#define GPIO_DEVICE_ID  XPAR_AXI_GPIO_0_DEVICE_ID    /* GPIO device that LEDs are connected to */
#define LED 0x00                                    /* Initial LED value - XX0000XX */
#define LED_DELAY 100000000                            /* Software delay length */
#define CLK_COUNT 50000000
#define LED_CHANNEL 1                                /* GPIO port for LEDs */
#define printf xil_printf                            /* smaller, optimised printf */
#define VGA_CONFIG_BASE_ADDRESS     0x43c00000      /* Control reg's for the VGA circuitry */
#define VGA_MEMORY_ATTRIBUTE 0x00010c06             /* Attribute applied to VGA frame buffer in DRAM. */

XGpio Gpio;

XUartPs Uart_Ps;        /* The instance of the UART Driver */
XUartPs_Config *Config;

vga vga_obj;
vga_frame vga_frame_obj;
vga_pixel vga_pixel_obj;

int w = 640, x, y;

int sec_arr[2] = {0,0}; // 0 is first digit ( least significant) , 1 is most significant
int min_arr[2] = {0,0};
int hrs_arr[2] = {0,0};

//ALL HEXADECIMAL SERIAL FOR THE MONITOR
int RGB_background = RBG_background_default;
int	RGB_seconds = RBG_seconds_default;
int	RGB_minutes = RBG_minutes_default;
int	RGB_hours = RBG_hours_default;
int RGB_Colon = RGB_COLON_default;
int seconds, minutes = 12,hours=5;



void outputGUI(int x, int y);
void BreakTime(int time_2digit,int* arr);
void sendInt( int x_counter,int y_counter,int ChangedChar,int Colors,int y_max,int y_min,int x_max, int x_min);
void sendColon(int x_counter,int y_counter,int Colors,int y_max,int y_min,int x_max, int x_min);
void console_init();
//void checkTime();
int main(){

    int Status;
    int led = LED; /* Hold current LED value. Initialise to LED definition */
    seconds = 0;
    //---------------  LEDS  --------------------------------------------------
    /* GPIO driver initialisation */
    Status = XGpio_Initialize(&Gpio, GPIO_DEVICE_ID);
    if (Status != XST_SUCCESS) {
        return XST_FAILURE;
    }
    /*Set the direction for the LEDs to output. */
    XGpio_SetDataDirection(&Gpio, LED_CHANNEL, 0x00);
    /* Write output to the LEDs. */
    XGpio_DiscreteWrite(&Gpio, LED_CHANNEL, led);
    //--------------  END OF LEDS  ---------------------------------------------

  //  print("Hello World\n\r");

     /* Configure VGA frame buffer memory to device. */
    Xil_SetTlbAttributes(VGA_FRAME_ADDRESS,VGA_MEMORY_ATTRIBUTE);

    /* Configure the vga object. */
    vga_setup(&vga_obj,(uint32_t*)VGA_CONFIG_BASE_ADDRESS,(vga_frame*)VGA_FRAME_ADDRESS);

    /* Clear the image frame, then draws a simple circle, then flushes the local buffer to VGA memory in DRAM for display. */
    vga_pixel_setup( &vga_pixel_obj, 0x4,0x4,0x4);
    vga_frame_clear( &vga_frame_obj);

    //Initialize UART

    Config = XUartPs_LookupConfig(UART_DEVICE_ID);
    XUartPs_CfgInitialize(&Uart_Ps, Config, Config->BaseAddress);
    XUartPs_SetBaudRate(&Uart_Ps, 115200);
        console_init();
    while (1){
        led = (led + 1) & 0xff;  // Increment the Zedboard LEDS each time the clock is re-drawn. To see the speed.
        //led = (led + 1);
        XGpio_DiscreteWrite(&Gpio, LED_CHANNEL, led);
        seconds = seconds+1;
        BreakTime(seconds,sec_arr);
        BreakTime(minutes,min_arr);
        BreakTime(hours,hrs_arr);
        int j=0;
        int m=0;
        for(j=0; j<74000000;j++){// one second delay
                m=1;
         }
        if (seconds > 59){
            minutes = minutes + 1;
            seconds = 0;
           // BreakTime(minutes,min_arr);
        }
        if (minutes > 59){
            hours = hours +1;
          //  BreakTime(hours,hrs_arr);

            minutes = 0;
        }
        if (hours >12){//tends to break so breaktime will fix it
        	//BreakTime(hours,hrs_arr);
            hours = 0;
        }
         for(y = 0; y < w;y++){
                for(x=0; x < w;x++){
                    outputGUI(x,y);
                    }
         }
    }
    return 0;

}

void console_init(){ //console print
    printf("Welcome to the magic clock: \n");
    printf("Set Seconds: \n");
	scanf("%d", &seconds);
	printf("\n Set Minutes : ");
	scanf("%d", &minutes);
	printf("\n Set Hours : ");
	scanf("%d", &hours);
}

void BreakTime(int time_2digit,int* arr){ // Will come out in reverse order [1] represents smaller digit [0] represents higher digit

    int value = time_2digit;
    int i = 0;
    int remainder = 0;
    if(value<10){
    	arr[0]=value;
    	arr[1]=0;
    }
    while (value > 0) {
        remainder = value % 10;
        value /= 10;
        arr[i] = remainder;
        i=i+1;
    }
}
/*
	time increasing slowly
		When it reaches two digits example 10:
			-> put smaller digit in arr[1] and higher in arr[0]
				-> keep arr[1] to changing numbers
				-> no change on [0] , keep that for BreakTime()
*/

//Outputs the GUI , given hrs_arr min_arr sec_arr
void outputGUI(int x, int y){
    sendInt(x,y,hrs_arr[1],RGB_hours,CharHOURS_Y_MAX,CharHOURS_Y_MIN,CharHOURS_X1_MAX,CharHOURS_X1_MIN);// On the left
    sendInt(x,y,hrs_arr[0],RGB_hours,CharHOURS_Y_MAX,CharHOURS_Y_MIN,CharHOURS_X_MAX,CharHOURS_X_MIN); // On the right
    sendColon(x,y,RGB_Colon,COLON_Y_MAX,COLON_Y_MIN,COLONLeft_X_MAX,COLONLeft_X_MIN);
    sendInt(x,y,min_arr[1],RGB_minutes,CharMIN_Y_MAX,CharMIN_Y_MIN,CharMIN_X1_MAX,CharMIN_X1_MIN);
    sendInt(x,y,min_arr[0],RGB_minutes,CharMIN_Y_MAX,CharMIN_Y_MIN,CharMIN_X_MAX,CharMIN_X_MIN);
    sendColon(x,y,RGB_Colon,COLON_Y_MAX,COLON_Y_MIN,COLONRight_X_MAX,COLONRight_X_MIN);
    sendInt(x,y,sec_arr[1],RGB_seconds,CharSECONDS_Y_MAX,CharSECONDS_Y_MIN,CharSECONDS_X1_MAX,CharSECONDS_X1_MIN);
    sendInt(x,y,sec_arr[0],RGB_seconds,CharSECONDS_Y_MAX,CharSECONDS_Y_MIN,CharSECONDS_X_MAX,CharSECONDS_X_MIN);

}
// Sends a Colon : to the given dimensions
void sendColon(int x_counter,int y_counter,int Colors,int y_max,int y_min,int x_max, int x_min){// Should not be responding well to color changes bec background overrides for now , fix later
    int var = y_max-y_min;
    if(x_counter > x_min && x_counter <x_max && y_counter > y_min+0.1*var && y_counter<y_min + 0.35*var){ // Top square or line
        vga_set_pixel( x_counter, y_counter, Colors);
    }
    else if(x_counter > x_min && x_counter <x_max && y_counter > y_max-0.35*var && y_counter<y_max-0.1*var){
        vga_set_pixel( x_counter, y_counter, Colors);
    }
}
//Sends a number on a given direction
void sendInt( int x_counter,int y_counter,int ChangedChar,int Colors,int y_max,int y_min,int x_max, int x_min){
    int var = 20; //Represents pixels of width
    switch(ChangedChar){ // maybe add else for background?
        case 0:
            if(x_counter > x_min && x_counter <x_max && y_counter > y_min && y_counter<y_min + var){// Top line of zero
                vga_set_pixel( x_counter, y_counter, Colors);
            }
            else if ( x_counter > x_min && x_counter <x_min+var && y_counter > y_min && y_counter<y_max){// Left line of zero
                vga_set_pixel( x_counter, y_counter, Colors);
            }
            else if( x_counter > x_max-var && x_counter <x_max && y_counter>y_min  &&  y_counter < y_max ){ //Right line of zero
                vga_set_pixel( x_counter, y_counter, Colors);

            }
            else if ( x_counter > x_min && x_counter <x_max && y_counter>y_max-var  &&  y_counter < y_max){//bottom
                vga_set_pixel( x_counter, y_counter, Colors);

            }
            else if (x_counter>x_min && x_counter<x_max && y_counter>y_min && y_counter<y_max){ //Nothing? Make it black
                vga_set_pixel( x_counter, y_counter, 0);
            }
            
        break;

        case 1:
            if(x_counter >=(x_min + (x_max-x_min)*0.3 )&& (x_counter <=x_max-((x_max-x_min)*0.3) )&& y_counter > y_min && y_counter<y_max ){//Straight line going down of 1
                vga_set_pixel( x_counter, y_counter, Colors);
            }

            else if (x_counter>x_min && x_counter<x_max && y_counter>y_min && y_counter<y_max){ //Nothing? Make it black
                vga_set_pixel( x_counter, y_counter, 0);
            }
        break;

        case 2:
            if(x_counter > x_min && x_counter <x_max && y_counter > y_min && y_counter<y_min + var){//Top horizontal line of 2
                vga_set_pixel( x_counter, y_counter, Colors);
            }
            else if(x_counter > x_max-var && x_counter <x_max && y_counter > y_min && y_counter<y_min+4*var){// Right top vertical line
                vga_set_pixel( x_counter, y_counter, Colors);
            }
            else if(x_counter > x_min && x_counter <x_max && y_counter >= y_min+4*var && y_counter<y_max-3*var){// Middle horizontal line
                vga_set_pixel( x_counter, y_counter, Colors);
            }
            else if(x_counter > x_min && x_counter <x_min+var && y_counter > y_max-4*var && y_counter<y_max){// Left vertical line
                vga_set_pixel( x_counter, y_counter, Colors);
            }
            else if(x_counter > x_min && x_counter <x_max && y_counter > y_max- var && y_counter<y_max){//bottom horizontal line
                vga_set_pixel( x_counter, y_counter, Colors);
            }else if (x_counter>x_min && x_counter<x_max && y_counter>y_min && y_counter<y_max){ //Nothing? Make it black
                vga_set_pixel( x_counter, y_counter, 0);
            }

        break;

        case 3:

            if(x_counter > x_min && x_counter <x_max && y_counter > y_min && y_counter<y_min + var){//Top horizontal line
                vga_set_pixel( x_counter, y_counter, Colors);
            }
            else if ( x_counter > x_min && x_counter <x_max && y_counter >  y_max-((y_max-y_min)*0.5)-0.5*var && y_counter< y_max-((y_max-y_min)*0.5) + 0.5*var){//Middle horizontal line
                vga_set_pixel( x_counter, y_counter, Colors);
            }
            else if (x_counter > x_min && x_counter <x_max && y_counter > y_max-var && y_counter<y_max){//Bottom horizontal line
                vga_set_pixel( x_counter, y_counter, Colors);
            }

            else if (x_counter > x_max-var && x_counter <x_max && y_counter > y_min && y_counter<y_max){//Vertical line
                vga_set_pixel( x_counter, y_counter, Colors);
            }else if (x_counter>x_min && x_counter<x_max && y_counter>y_min && y_counter<y_max){ //Nothing? Make it black
                vga_set_pixel( x_counter, y_counter, 0);
            }
        break;

        case 4:
            if(x_counter > x_min && x_counter <x_min+var && y_counter > y_min && y_counter<=y_max-((y_max-y_min)*0.5)){//Left vertical
                vga_set_pixel( x_counter, y_counter, Colors);
            }
            else if( x_counter > x_min && x_counter <x_max && y_counter >  y_max-((y_max-y_min)*0.5) && y_counter< y_max-((y_max-y_min)*0.5) + var){//Middle horizontal
                vga_set_pixel( x_counter, y_counter, Colors);
            }
            else if(x_counter > x_max-var && x_counter <x_max && y_counter > y_min && y_counter<y_max){//Right Vertical
                vga_set_pixel( x_counter, y_counter, Colors);
            }
            else if (x_counter>x_min && x_counter<x_max && y_counter>y_min && y_counter<y_max){ //Nothing? Make it black
                            vga_set_pixel( x_counter, y_counter, 0);
                        }
        break;

        case 5:
            if(x_counter > x_min && x_counter <x_max && y_counter > y_min && y_counter<y_min + var){//top horizontal
                vga_set_pixel( x_counter, y_counter, Colors);
            }
            else if(x_counter > x_min && x_counter <x_min+var && y_counter > y_min && y_counter<=y_max-((y_max-y_min)*0.5)){//left vertical
                vga_set_pixel( x_counter, y_counter, Colors);
            }
            else if( x_counter > x_min && x_counter <x_max && y_counter >  y_max-((y_max-y_min)*0.5) && y_counter< y_max-((y_max-y_min)*0.5) + var){//middle horizontal
                vga_set_pixel( x_counter, y_counter, Colors);
            }
            else if(x_counter > x_max-var && x_counter <x_max && y_counter > y_max-((y_max-y_min)*0.5) && y_counter<y_max){//right vertical
                vga_set_pixel( x_counter, y_counter, Colors);
            }
            else if( x_counter > x_min && x_counter <x_max && y_counter >  y_max-var && y_counter< y_max){//bottom horizontal
                vga_set_pixel( x_counter, y_counter, Colors);
            }
            else if (x_counter>x_min && x_counter<x_max && y_counter>y_min && y_counter<y_max){ //Nothing? Make it black
                            vga_set_pixel( x_counter, y_counter, 0);
                        }
        break;

        case 6:
        if(x_counter > x_min && x_counter <x_min+var && y_counter > y_min && y_counter<y_max){//Left vertical
            vga_set_pixel( x_counter, y_counter, Colors);
        }
        else if(x_counter > x_min && x_counter <x_max && y_counter > y_min && y_counter<y_min + var){//Top horizontal
            vga_set_pixel( x_counter, y_counter, Colors);
        }
        else if(x_counter > x_min && x_counter <x_max && y_counter > y_max-((y_max-y_min)*0.5) && y_counter<y_max-((y_max-y_min)*0.5)  + var){//Middle horizontal
            vga_set_pixel( x_counter, y_counter, Colors);
        }
        else if(x_counter > x_max-var && x_counter <x_max && y_counter > y_max-((y_max-y_min)*0.5) && y_counter<y_max){//right vertical
            vga_set_pixel( x_counter, y_counter, Colors);
        }
        else if(x_counter > x_min && x_counter <x_max && y_counter >  y_max-var && y_counter< y_max){//bottom horizontal
            vga_set_pixel( x_counter, y_counter, Colors);
        }
        else if (x_counter>x_min && x_counter<x_max && y_counter>y_min && y_counter<y_max){ //Nothing? Make it black
                        vga_set_pixel( x_counter, y_counter, 0);
                    }
        break;

        case 7:
        if(x_counter > x_min && x_counter <x_max && y_counter > y_min && y_counter<y_min + var){ //Top horizontal
            vga_set_pixel( x_counter, y_counter, Colors);
        }
        else if(x_counter > x_max-var && x_counter <x_max && y_counter > y_min && y_counter<y_max){//right vertical
            vga_set_pixel( x_counter, y_counter, Colors);
        }
        else if (x_counter>x_min && x_counter<x_max && y_counter>y_min && y_counter<y_max){ //Nothing? Make it black
                        vga_set_pixel( x_counter, y_counter, 0);
                    }

        break;

        case 8:
        if(x_counter > x_min && x_counter <x_max && y_counter > y_min && y_counter<y_min + var){// Top horizontal
            vga_set_pixel( x_counter, y_counter, Colors);
        }
        else if(x_counter > x_min && x_counter <x_max && y_counter > y_max-((y_max-y_min)*0.5) && y_counter<y_max-((y_max-y_min)*0.5)  + var){//Middle horizontal
            vga_set_pixel( x_counter, y_counter, Colors);
        }
        else if(x_counter > x_min && x_counter <x_max && y_counter >  y_max-var && y_counter< y_max){//Bottom horizontal
            vga_set_pixel( x_counter, y_counter, Colors);
        }
        else if(x_counter > x_min && x_counter <x_min+var && y_counter > y_min && y_counter<y_max){//Left vertical
            vga_set_pixel( x_counter, y_counter, Colors);
        }
        else if(x_counter > x_max-var && x_counter <x_max && y_counter > y_min && y_counter<y_max){//Right vertical
            vga_set_pixel( x_counter, y_counter, Colors);
        }
        else if (x_counter>x_min && x_counter<x_max && y_counter>y_min && y_counter<y_max){ //Nothing? Make it black
                        vga_set_pixel( x_counter, y_counter, 0);
                    }
        break;

        case 9:
        if(x_counter > x_min && x_counter <x_max && y_counter > y_min && y_counter<y_min + var){// Top horizontal
            vga_set_pixel( x_counter, y_counter, Colors);
        }
        else if(x_counter > x_min && x_counter <x_max && y_counter > y_max-((y_max-y_min)*0.5) && y_counter<y_max-((y_max-y_min)*0.5)  + var){//Middle horizontal
            vga_set_pixel( x_counter, y_counter, Colors);
        }
        else if(x_counter > x_min && x_counter <x_max && y_counter >  y_max-var && y_counter< y_max){//Bottom horizontal
            vga_set_pixel( x_counter, y_counter, Colors);
        }
        else if(x_counter > x_min && x_counter <x_min+var && y_counter > y_min && y_counter<=y_max-((y_max-y_min)*0.5)){//Left vertical
            vga_set_pixel( x_counter, y_counter, Colors);
        }
        else if(x_counter > x_max-var && x_counter <x_max && y_counter > y_min && y_counter<y_max){//Right vertical
            vga_set_pixel( x_counter, y_counter, Colors);
        }
        else if (x_counter>x_min && x_counter<x_max && y_counter>y_min && y_counter<y_max){ //Nothing? Make it black
                        vga_set_pixel( x_counter, y_counter, 0);
                    }
        break;
        default:
            if (x_counter>x_min && x_counter<x_max && y_counter>y_min && y_counter<y_max){ //Nothing? Make it black
                    vga_set_pixel( x_counter, y_counter, 0);
                }
            break;
    }
}
