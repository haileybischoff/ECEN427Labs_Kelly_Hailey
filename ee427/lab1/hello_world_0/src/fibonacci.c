#include <stdio.h>
#include "platform.h"
#include <stdint.h>

#define NUMBER_OF_ITERATIONS 15

int main() {
    init_platform();

	uint16_t previous = 0;
	uint16_t current = 1;
	uint16_t summed = 0;
	uint16_t i = 0;
	for(i; i <= NUMBER_OF_ITERATIONS; i++){
		xil_printf("%d, ", current);
		summed = previous + current;
		previous = current;
		current = summed;
	}
	xil_printf("\n\r");
    cleanup_platform();
	return 0;

}
