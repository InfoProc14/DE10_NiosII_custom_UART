#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "system.h"
#include "alt_types.h"
#include "sys/alt_irq.h"
#include "altera_up_avalon_accelerometer_spi.h"
#include "altera_avalon_timer_regs.h"
#include "altera_avalon_timer.h"
#include "altera_avalon_pio_regs.h"

int transmission = 0;

void setup_button_interrupts(void* isr) {
    IOWR_ALTERA_AVALON_PIO_EDGE_CAP(BUTTON_BASE, 0xFF);     // clear capture register by writing 1
    IOWR_ALTERA_AVALON_PIO_IRQ_MASK(BUTTON_BASE, 0x3);      // enable interrupts for both KEY buttons
    alt_irq_register(BUTTON_IRQ, 0, isr);
}

void button_isr(void* context) {
    alt_u32 buttons = IORD_ALTERA_AVALON_PIO_EDGE_CAP(BUTTON_BASE); // read button capture
    IOWR_ALTERA_AVALON_PIO_EDGE_CAP(BUTTON_BASE, 0xFF);             // reset button capture
    if (buttons & 0b01) {                                           // KEY 0 PRESSED
        transmission = 1;
        alt_putstr("BUTTON ISR: Transmission Enabled.\n");
    }
    else if (buttons & 0b10) {                                      // KEY 1 PRESSED
        transmission = 0;
        alt_putstr("BUTTON ISR: Transmission Disabled.\n");
    }
    else {
      alt_putstr("BUTTON ISR: Unrecognised.\n");
    }
}

int main() {

    alt_32 x_read;
    alt_32 y_read;
    alt_32 z_read;
    char* message;

    alt_up_accelerometer_spi_dev* acc_dev = alt_up_accelerometer_spi_open_dev("/dev/accelerometer_spi");
    FILE* fp = fopen("/dev/uart", "r+");

    if (fp == NULL || acc_dev == NULL) {
        alt_putstr("FAILURE: `/dev/accelerometer_spi` or `/dev/uart` failed to open.");
    } else {
        alt_putstr("SUCCESS: `/dev/accelerometer_spi` and `/dev/uart` opened successfully.\n");
    }

    setup_button_interrupts(button_isr);

    while (1) {
        if(transmission) {
            alt_up_accelerometer_spi_read_x_axis(acc_dev, &x_read);
            alt_up_accelerometer_spi_read_y_axis(acc_dev, &y_read);
            alt_up_accelerometer_spi_read_z_axis(acc_dev, &z_read);
            sprintf(message, "%d, %d, %d\n", x_read, y_read, z_read);
            fwrite(message, strlen(message), 1, fp);
        }
    }

    alt_putstr("Closing the UART file.\n"); 
    fclose(fp);
    return 0;
}