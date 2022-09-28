#include "contiki.h"
#include "dev/serial-line.h"
#include "buzzer.h"
#include <stdio.h> /* For printf() */
#include "dev/cc26xx-uart.h"
#include "ieee-addr.h"
#include "dev/leds.h"

/*---------------------------------------------------------------------------*/
PROCESS(process1, "Serial line test process");
AUTOSTART_PROCESSES(&process1);
/*---------------------------------------------------------------------------*/

static bool shouldToggleR = false;
static bool shouldToggleG = false;
static int buz_freq=500;
static bool buz_state=false;
static bool buz_to_turn=false;
static int to_increase=0;
static int to_decrease=0;
unsigned char ieee_addr[8];

static struct ctimer timer_ctimer;
static struct ctimer timer_ctimer2; // for buzzer volume change

void do_timeout();
void do_timeout2();

//Serial Interface
PROCESS_THREAD(process1, ev, data) {

	PROCESS_BEGIN();

	cc26xx_uart_set_input(serial_line_input_byte);	//Initalise UART in serial driver
	ctimer_set(&timer_ctimer, 2* CLOCK_SECOND, do_timeout, NULL);
	ctimer_set(&timer_ctimer2, 1* CLOCK_SECOND, do_timeout2, NULL);
	ieee_addr_cpy_to(ieee_addr, 8);
	
   	while(1) {

     	PROCESS_YIELD();	//Let other threads run

		//Wait for event triggered by serial input
     	
		//******************************************
		//NOTE: MUST HOLD CTRL and then press ENTER 
		//at the end of typing for the serial driver 
		//to work. Serial driver expects 0x0A as
		//last character, to tigger the event.
		//******************************************
		if(ev == serial_line_event_message) {
       			//printf("received line: %s\n\r", (char *)data);
			switch(*(char*)data){
				case 'r':
					shouldToggleR = !shouldToggleR;
					break;
				case 'g':
					shouldToggleG = !shouldToggleG;
					break;
				case 'a':{
					shouldToggleR = !shouldToggleR;
					shouldToggleG = !shouldToggleG;
					break;
					}
				case 'b':
					buz_to_turn = true;
					break;
				case 'i':
					to_increase = 3;
					break;
				case 'd':
					to_decrease = 3;
					break;
				case 'n':
					for(int i = 0; i<8;i++)
					    printf("%02hhX ", ieee_addr[i]);
					printf("\n");
			}

     		}
   	}
   	PROCESS_END();
}


void do_timeout() {
  ctimer_reset(&timer_ctimer);
  //printf("\n\rCTimer1 callback called");
  //buzzer_start(500);
  //clock_wait(CLOCK_SECOND);	//Wait for 1 second
  //buzzer_stop();
  if(shouldToggleR)
	leds_toggle(LEDS_RED);
  if(shouldToggleG)
	leds_toggle(LEDS_GREEN);
  if(buz_to_turn == true){
	//printf("turning buzzer\n");		
	buz_state = !buz_state;
	buz_to_turn = false;
	if(buz_state){
		buzzer_start(buz_freq);
		//printf("started buzzer");
	}
	else
		buzzer_stop();
  }
}

void do_timeout2() {
	ctimer_reset(&timer_ctimer2);
	//printf("\n\rCTimer2 callback called");
	if(to_increase > 0){
		to_increase -= 1;
		buzzer_stop();
		buz_freq += 100;
		buzzer_start(buz_freq);
	}
	if(to_decrease > 0){
		to_decrease -= 1;
		buzzer_stop();
		buz_freq -= 100;
		buzzer_start(buz_freq);
	}
}
	

