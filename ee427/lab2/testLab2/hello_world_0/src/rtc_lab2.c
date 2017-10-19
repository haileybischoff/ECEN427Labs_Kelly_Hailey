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
#define HOUR_BUTTON_MASK 8 //This is the hour mask
#define MINUTE_BUTTON_MASK 1 //This is the minute mask
#define SECOND_BUTTON_MASK 2 //This is the second mask
#define INCREMENT_BUTTON_MASK 16 //This is the increment mask
#define DECREMENT_BUTTON_MASK 4 //This is the decrement mask
#define HOURS_TURNOVER 24 //Total hours is 24
#define HOURS_RESET 23 //Start hours over at 23
#define MINUTES_AND_SECONDS_TURNOVER 60 //Total time for minutes and seconds
#define MINUTES_AND_SECONDS_RESET 59 //Start minutes and seconds over at 59
#define RESET 0 //Reset is 0

XGpio gpLED;  // This is a handle for the LED GPIO block.
XGpio gpPB;   // This is a handle for the push-button GPIO block.
int currentButtonState;		// Value the button interrupt handler saves button values to
int debouncedButtonState = 0;	// Saves the debounced button states
uint16_t seconds = 0; //Create seconds at 0
uint16_t minutes = 0; //Create minutes at 0
uint16_t hours = 0; //Create hours at 0
uint16_t mili_count = 0; //Timer for seconds
uint16_t btn_count; //Timer for buttons
uint16_t auto_count = 0; //Timer for auto increment
uint16_t print_count = 0; //Timer for print

void debounce_buttons(){ //this takes the button and debounces it for us
	debouncedButtonState = currentButtonState & 0x0000001F;
}

void Increment_hours(){ //This increments hours
	hours++; //Increment hours
	if(hours >= HOURS_TURNOVER){ //Hours at max
		hours = RESET; //Reset hours
	}
}

void Decrement_hours(){ //This decrements hours
	if(hours == RESET){ //Hours is 0
		hours = HOURS_RESET; //Hours are 23
	}
	else{
		hours --; //Decrement hours
	}
}

void Increment_minutes(){
	minutes++; //Increment minutes
	if(minutes >= MINUTES_AND_SECONDS_TURNOVER){ //Minutes is max
		minutes = RESET; //Reset minutes
		if(debouncedButtonState == 0){ //If buttons are not pressed
			Increment_hours(); //Update hours
		}
	}
}

void Decrement_minutes(){
	if(minutes == RESET){ //Minutes are 0
		minutes = MINUTES_AND_SECONDS_RESET; //Minutes are not 29
	}
	else{
		minutes --; //Decrement minutes
	}
}

void Increment_seconds(){
	seconds++; //Increment seconds
	if(seconds >= MINUTES_AND_SECONDS_TURNOVER){ //Seconds are max
		seconds = RESET; //Reset seconds
		if(debouncedButtonState == 0){ //Buttons are not pressed
			Increment_minutes(); //Increment minutes
		}
	}
}

void Decrement_seconds(){
	if(seconds == RESET){ //Seconds are max
		seconds = MINUTES_AND_SECONDS_RESET; //Seconds are 59
	}
	else{
		seconds --; //Decrement seconds
	}
}


void update_clock(){
	if(debouncedButtonState & INCREMENT_BUTTON_MASK){ //Increment is pressed
		if(debouncedButtonState & HOUR_BUTTON_MASK){ //Hours are pressed
			Increment_hours(); //Increment hours
		}
		if(debouncedButtonState & MINUTE_BUTTON_MASK){ //Minutes are pressed
			Increment_minutes(); //Increment minutes
		}
		if(debouncedButtonState & SECOND_BUTTON_MASK){ //Seconds are pressed
			Increment_seconds(); //Increment seconds
		}
	}
	if(debouncedButtonState & DECREMENT_BUTTON_MASK){ //Decrement is pressed
		if(debouncedButtonState & HOUR_BUTTON_MASK){ //Hours are pressed
			Decrement_hours(); //Decrement hours
		}
		if(debouncedButtonState & MINUTE_BUTTON_MASK){ //Minutes are pressed
			Decrement_minutes(); //Decrement minutes
		}
		if(debouncedButtonState & SECOND_BUTTON_MASK){ //Seconds are pressed
			Decrement_seconds(); //Decrement seconds
		}
	}
}

void timer_interrupt_handler(){
	mili_count++; //Increment milisecond count
	auto_count++; //Increment auto count
	print_count++; //Increment print count
	if(mili_count >= 100){ //Timer reaches one second
		if(debouncedButtonState == RESET){ //No buttons are pressed
			Increment_seconds(); //Increment seconds
		}
		mili_count = RESET; //Reset milisecond count
	}
	if(print_count >= 20){ //Print count is over 20
		xil_printf("\r%02d:%02d:%02d", hours, minutes, seconds); //Print time
		print_count = 0; //Reset print counter
	}
	if(btn_count == 7){
		debounce_buttons(); //Debounce buttons
		if(debouncedButtonState > 0){ //Buttons are pushed
			update_clock(); //Update clock
		}
	}
	if(btn_count >= 100){ //Button count is more than a second
		if(auto_count >= 50){ //Auto is more than 50
			update_clock(); //Update clock
			//xil_printf("\r%02d:%02d:%02d", hours, minutes, seconds); //Print time
			auto_count = 0; //Reset auto
		}
	}
	btn_count++; //Increment button count
}

// This is invoked each time there is a change in the button state (result of a push or a bounce).
void pb_interrupt_handler() {
  // Clear the GPIO interrupt.
  XGpio_InterruptGlobalDisable(&gpPB);                // Turn off all PB interrupts for now.
  currentButtonState = XGpio_DiscreteRead(&gpPB, 1);  // Get the current state of the buttons.
  // Reset button counter
  btn_count = 0;
  XGpio_InterruptClear(&gpPB, 0xFFFFFFFF);            // Ack the PB interrupt.
  XGpio_InterruptGlobalEnable(&gpPB);                 // Re-enable PB interrupts.
}

// Main interrupt handler, queries the interrupt controller to see what peripheral
// fired the interrupt and then dispatches the corresponding interrupt handler.
// This routine acks the interrupt at the controller level but the peripheral
// interrupt must be ack'd by the dispatched interrupt handler.
void interrupt_handler_dispatcher(void* ptr) {
	int intc_status = XIntc_GetIntrStatus(XPAR_INTC_0_BASEADDR);
	// Check the FIT interrupt first.
	if (intc_status & XPAR_FIT_TIMER_0_INTERRUPT_MASK){
		XIntc_AckIntr(XPAR_INTC_0_BASEADDR, XPAR_FIT_TIMER_0_INTERRUPT_MASK);
		timer_interrupt_handler();
	}
	// Check the push buttons.
	if (intc_status & XPAR_PUSH_BUTTONS_5BITS_IP2INTC_IRPT_MASK){
		XIntc_AckIntr(XPAR_INTC_0_BASEADDR, XPAR_PUSH_BUTTONS_5BITS_IP2INTC_IRPT_MASK);
		pb_interrupt_handler();
	}
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
