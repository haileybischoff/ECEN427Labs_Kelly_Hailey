/*
 * rtc_lab2.c
 *
 *  Created on: Sep 21, 2017
 *      Author: superman
 */
#include <stdbool.h>
#include "xgpio.h"          // Provides access to PB GPIO driver.
#include <stdio.h>          // xil_printf and so forth.
#include "platform.h"       // Enables caching and other system stuff.
#include "mb_interface.h"   // provides the microblaze interrupt enables, etc.
#include "xintc_l.h"        // Provides handy macros for the interrupt controller.
#include <stdint.h>
#define ONE_SECOND 100 // 1 second or (100 miliseconds * 10) //TODO
#define RESET 0 //
#define TRUE 1
#define FALSE 0
#define HOURS_TURNOVER 24
#define HOURS_RESET 23
#define MINUTES_AND_SECONDS_TURNOVER 60
#define MINUTES_AND_SECONDS_RESET 59
#define HOUR_BUTTON_MASK 8
#define MINUTE_BUTTON_MASK 1
#define SECOND_BUTTON_MASK 2
#define INITIAL_HOURS 23
#define INITIAL_MINUTES 59
#define INITIAL_SECONDS 55
#define INCREMENT_BUTTON_MASK 16
#define DECREMENT_BUTTON_MASK 4
#define DEBOUNCE_TIME 6 // 5 miliseconds

XGpio gpLED;  // This is a handle for the LED GPIO block.
XGpio gpPB;   // This is a handle for the push-button GPIO block.

enum Timer_State {init, idle, update_time, debounce, check_value, button_update_time} current_state = init;

static u32 currentButtonState = RESET; //static u32 currentButtonState = RESET;
static u32 previousButtonState = RESET; //static u32 previousButtonState = RESET;
static uint8_t button_pushed_flag = FALSE; //static int button_pushed_flag = FALSE;
u32 getCurrentButtonState();

// This is invoked in response to a timer interrupt.
// It does 2 things: 1) debounce switches, and 2) advances the time.
void timer_interrupt_handler() {
	u32 myButtonValue = getCurrentButtonState();
	uint8_t count_milisec; //	int count_milisec;
	uint8_t button_flag; // 	int button_flag;
	uint8_t hours; //	int hours;
	uint8_t minutes; //	int minutes;
	uint8_t seconds; //	int seconds;
	uint8_t debounce_count; //	int debounce_count;
	bool hour_flag; //	int hour_flag;
	bool min_flag; //	int min_flag;
	bool sec_flag; //	int sec_flag;
	bool decrement_flag; //	int decrement_flag;
	bool increment_flag; //	int increment_flag;

	// Action states
	switch (current_state){
	case init:
		count_milisec = RESET;
		button_flag = FALSE;
		hours = INITIAL_HOURS;
		minutes = INITIAL_MINUTES;
		seconds = INITIAL_SECONDS;
		debounce_count = RESET;
		hour_flag = FALSE;
		min_flag = FALSE;
		sec_flag = FALSE;
		decrement_flag = FALSE;
		increment_flag = FALSE;
		break;
	case idle:
		count_milisec++;
		debounce_count = RESET;

		break;
	case update_time:
		//if(hour_flag || min_flag || sec_flag){

		//}

			seconds++;
			if(seconds >= MINUTES_AND_SECONDS_TURNOVER){
				minutes++;
				seconds = RESET;
			}
			if(minutes >= MINUTES_AND_SECONDS_TURNOVER){
				hours++;
				minutes = RESET;
			}
			if(hours >= HOURS_TURNOVER){
				hours = RESET;
			}
		//}
		xil_printf("\r%02d:%02d:%02d", hours, minutes, seconds);
		break;
	case debounce:
//		if(button_pushed_flag == TRUE){
//			debounce_count = RESET;
//		}
		if(myButtonValue != 0){
			debounce_count++;
		}//else{
		//	debounce_count = RESET;
		//}
		break;
	case check_value:
		hour_flag = FALSE;
		min_flag = FALSE;
		sec_flag = FALSE;
		decrement_flag = FALSE;
		increment_flag = FALSE;
		if(myButtonValue & HOUR_BUTTON_MASK){
			hour_flag = TRUE;
		}
		if(myButtonValue & MINUTE_BUTTON_MASK){
			min_flag = TRUE;
		}
		if(myButtonValue & SECOND_BUTTON_MASK){
			sec_flag = TRUE;
		}
		if(myButtonValue & DECREMENT_BUTTON_MASK){
			decrement_flag = TRUE;
		}
		if(myButtonValue & INCREMENT_BUTTON_MASK){
			increment_flag = TRUE;
		}
		break;
	case button_update_time:
		if(increment_flag && !decrement_flag){
			if(hour_flag){
				hours++;
				if(hours >= HOURS_TURNOVER){
					hours = RESET;
				}
			}
			if(min_flag){
				minutes++;
				if(minutes >= MINUTES_AND_SECONDS_TURNOVER){
					minutes = RESET;
				}
			}
			if(sec_flag){
				seconds++;
				if(seconds >= MINUTES_AND_SECONDS_TURNOVER){
					seconds = RESET;
				}
			}
		}
		if(decrement_flag && !increment_flag){
			if(hour_flag){
				if(hours == RESET){
					hours = HOURS_RESET;
				}
				else{
					hours--;
				}
			}
			if(min_flag){
				if(minutes == RESET){
					minutes = MINUTES_AND_SECONDS_RESET;
				}
				else{
					minutes--;
				}
			}
			if(sec_flag){
				if(seconds == RESET){
					minutes = MINUTES_AND_SECONDS_RESET;
				}
				else{
					seconds--;
				}
			}
		}
		xil_printf("\r%02d:%02d:%02d", hours, minutes, seconds);
		break;
	default:
		break;
	}

	// Transitions
	switch (current_state){
	case init:
		current_state = idle;
		break;
	case idle:
		if(button_pushed_flag == TRUE){
			button_pushed_flag = FALSE;
			debounce_count = RESET;
			current_state = debounce;
		}
		else if(count_milisec >= ONE_SECOND){
			count_milisec = RESET;
			current_state = update_time;
		}
		break;
	case update_time:
		current_state = idle;
		break;
	case debounce:
		if(debounce_count == DEBOUNCE_TIME){
			//button_pushed_flag = TRUE; // Setting the button was pushed flag. added by kelly
			current_state = check_value;
		}
		if(myButtonValue == 0){
			count_milisec = RESET;
			button_pushed_flag = FALSE;
			current_state = idle;
		}
		//else{
		//	current_state = update_time;
		//}
		break;
	case check_value:
	//	if(button_pushed_flag == TRUE){
		//	button_pushed_flag = FALSE;
			//debounce_count = RESET;
	//		current_state = debounce;
		//}
		//else{
			button_pushed_flag = false;
			current_state = button_update_time;
	//	}
		break;
	case button_update_time:
		count_milisec = RESET;
		current_state = idle;
		break;
	default:
		xil_printf("bad stuff happened\n\r");
		current_state = init;
		break;
	}

}

u32 getCurrentButtonState(){
	return currentButtonState;
}

bool getButtonPushedFlag(){
	return button_pushed_flag;
}

// This is invoked each time there is a change in the button state (result of a push or a bounce).
void pb_interrupt_handler() {
  // disable all of the interrupts in the master enable registers.
//  XIntc_MasterDisable(XPAR_MICROBLAZE_0_INTC_BASEADDR);
  // Clear the GPIO interrupt.
  XGpio_InterruptGlobalDisable(&gpPB);                // Turn off all PB interrupts for now.
  currentButtonState = XGpio_DiscreteRead(&gpPB, 1);  // Get the current state of the buttons.
  // You need to do something here.
  // Restart the debounce timer here.
  //if((currentButtonState != previousButtonState) && (currentButtonState > previousButtonState)){
	//  previousButtonState = currentButtonState;
	  button_pushed_flag = TRUE; // Setting the button was pushed flag.
  //}
  //else if(currentButtonState != previousButtonState){
//	  previousButtonState = currentButtonState;
 // }
  XGpio_InterruptClear(&gpPB, 0xFFFFFFFF);            // Ack the PB interrupt.
  XGpio_InterruptGlobalEnable(&gpPB);                 // Re-enable PB interrupts.
  // enable all of the interrupts in the master enable register
//  XIntc_MasterEnable(XPAR_MICROBLAZE_0_INTC_BASEADDR);
}

// Main interrupt handler, queries the interrupt controller to see what peripheral
// fired the interrupt and then dispatches the corresponding interrupt handler.
// This routine acks the interrupt at the controller level but the peripheral
// interrupt must be ack'd by the dispatched interrupt handler.
// Question: Why is the timer_interrupt_handler() called after ack'ing the interrupt controller
// but pb_interrupt_handler() is called before ack'ing the interrupt controller?
void interrupt_handler_dispatcher(void* ptr) {
	XIntc_MasterDisable(XPAR_MICROBLAZE_0_INTC_BASEADDR);
	int intc_status = XIntc_GetIntrStatus(XPAR_INTC_0_BASEADDR);
	// Check the FIT interrupt first.
	if (intc_status & XPAR_FIT_TIMER_0_INTERRUPT_MASK){
		XIntc_AckIntr(XPAR_INTC_0_BASEADDR, XPAR_FIT_TIMER_0_INTERRUPT_MASK);
		timer_interrupt_handler();
	}
	// Check the push buttons.
	if (intc_status & XPAR_PUSH_BUTTONS_5BITS_IP2INTC_IRPT_MASK){
		pb_interrupt_handler();
		XIntc_AckIntr(XPAR_INTC_0_BASEADDR, XPAR_PUSH_BUTTONS_5BITS_IP2INTC_IRPT_MASK);
	}
	XIntc_MasterEnable(XPAR_MICROBLAZE_0_INTC_BASEADDR);
}

int main (void) {
    init_platform();
    // Initialize the GPIO peripherals.
    int success;
    success = XGpio_Initialize(&gpPB, XPAR_PUSH_BUTTONS_5BITS_DEVICE_ID);
    // Set the push button peripheral to be inputs.
    XGpio_SetDataDirection(&gpPB, 1, 0x0000001F);
    // Enable the global GPIO interrupt for push buttons.
    XGpio_InterruptGlobalEnable(&gpPB);
    // Enable all interrupts in the push button peripheral.
    XGpio_InterruptEnable(&gpPB, 0xFFFFFFFF);

    microblaze_register_handler(interrupt_handler_dispatcher, NULL);
    XIntc_EnableIntr(XPAR_INTC_0_BASEADDR,
    		(XPAR_FIT_TIMER_0_INTERRUPT_MASK | XPAR_PUSH_BUTTONS_5BITS_IP2INTC_IRPT_MASK));
    XIntc_MasterEnable(XPAR_INTC_0_BASEADDR);
    microblaze_enable_interrupts();

    while(1);  // Program never ends.

    cleanup_platform();

    return 0;
}