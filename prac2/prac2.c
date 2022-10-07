#include "contiki.h"
#include "dev/serial-line.h"
#include "buzzer.h"
#include <stdio.h> /* For printf() */
#include "dev/cc26xx-uart.h"
#include "ieee-addr.h"
#include "dev/leds.h"

#include "contiki-net.h"
#include "sys/cc.h"
#include <stdlib.h>
#include <string.h>

#include "dev/watchdog.h"
#include "random.h"
#include "board-peripherals.h"
#include "ti-lib.h"


#define SERVER_PORT 8080

static struct tcp_socket socket;

#define INPUTBUFSIZE 400
static uint8_t inputbuf[INPUTBUFSIZE];

#define OUTPUTBUFSIZE 400
static uint8_t outputbuf[OUTPUTBUFSIZE];

static uint8_t get_received;
static int bytes_to_send;

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
static int pressure_samp_to_take = 0;
static int humidity_samp_to_take = 0;
static int humidity_val;
static int pressure_val;
static char str[40];

static struct ctimer timer_ctimer;
static struct ctimer timer_ctimer2; // for buzzer volume change

void do_timeout();
void do_timeout2();

static struct ctimer hdc_timer;		//Callback timer
static struct ctimer bmp_timer;		//Callback timer

//Intialise Humidity sensor
static void hdc_init(void *not_used) {
	ctimer_reset(&hdc_timer);
	SENSORS_ACTIVATE(hdc_1000_sensor);
	humidity_val = hdc_1000_sensor.value(HDC_1000_SENSOR_TYPE_HUMIDITY);
}

static void bmp_init(void *not_used) {
	ctimer_reset(&bmp_timer);
	SENSORS_ACTIVATE(bmp_280_sensor);
	pressure_val = bmp_280_sensor.value(BMP_280_SENSOR_TYPE_PRESS);
}

/*---------------------------------------------------------------------------*/
//Input data handler
static int input(struct tcp_socket *s, void *ptr, const uint8_t *inputptr, int inputdatalen) {

 	printf("input %d bytes '%s'\n\r", inputdatalen, inputptr);

	tcp_socket_send_str(&socket, inputptr);	//Reflect byte

	switch(*(char*)inputptr){
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
		case 'b': {
			buz_to_turn = true;
			if(buz_to_turn == true){
				//printf("turning buzzer\n");		
				buz_state = !buz_state;
				buz_to_turn = false;
			}
			if(buz_state){
				buzzer_start(buz_freq);
				//printf("started buzzer");
			}
			else
				buzzer_stop();
			break;
		}
		case 'i':
			to_increase = 2;
			break;
		case 'd':
			to_decrease = 2;
			break;
		case 'p':
			pressure_samp_to_take = 5;
			break;
		case 'h':
			humidity_samp_to_take = 5;
			break;
		//case 'n':
		//	for(int i = 0; i<8;i++)
		//	    printf("%02hhX ", ieee_addr[i]);
		//	printf("\n");
	}


	//Clear buffer
	memset(inputptr, 0, inputdatalen);
    return 0; // all data consumed 
}

/*---------------------------------------------------------------------------*/
//Event handler
static void event(struct tcp_socket *s, void *ptr, tcp_socket_event_t ev) {
  printf("event %d\n", ev);
}


PROCESS_THREAD(process1, ev, data) {

  	PROCESS_BEGIN();

	cc26xx_uart_set_input(serial_line_input_byte);	//Initalise UART in serial driver
	ctimer_set(&timer_ctimer, 2* CLOCK_SECOND, do_timeout, NULL);
	ctimer_set(&timer_ctimer2, 1* CLOCK_SECOND, do_timeout2, NULL);
	ctimer_set(&hdc_timer, CLOCK_SECOND*1, hdc_init, NULL);	//Callback timer for humidity sensor
	ctimer_set(&bmp_timer, CLOCK_SECOND*1, bmp_init, NULL);	//Callback timer for pressure sensor
	ieee_addr_cpy_to(ieee_addr, 8);
	SENSORS_ACTIVATE(hdc_1000_sensor);
	SENSORS_ACTIVATE(bmp_280_sensor);

	//Register TCP socket
  	tcp_socket_register(&socket, NULL,
               inputbuf, sizeof(inputbuf),
               outputbuf, sizeof(outputbuf),
               input, event);
  	tcp_socket_listen(&socket, SERVER_PORT);

	printf("Listening on %d\n", SERVER_PORT);
	
	while(1) {
	
		//Wait for event to occur
		PROCESS_PAUSE();
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

}

void do_timeout2() {
	ctimer_reset(&timer_ctimer2);
	//printf("\n\rCTimer2 callback called");
	if(to_increase > 0){
		to_increase -= 1;
		buzzer_stop();
		buz_freq += 200;
		buzzer_start(buz_freq);
	}
	if(to_decrease > 0){
		to_decrease -= 1;
		buzzer_stop();
		buz_freq -= 200;
		buzzer_start(buz_freq);
	}
	if(humidity_samp_to_take > 0){
		humidity_samp_to_take -= 1;
		sprintf(str, "Humidity=%d.%02d \n\r", humidity_val/100, humidity_val%100);
		tcp_socket_send_str(&socket, str);	//Reflect byte
	}
	if(pressure_samp_to_take > 0){
		pressure_samp_to_take -= 1;
		sprintf(str, "Pressure=%d.%02d \n\r", pressure_val/100, pressure_val%100);
		tcp_socket_send_str(&socket, str);	//Reflect byte
	}
}