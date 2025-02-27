#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stddef.h>
#include <fcntl.h>

#include "system.h"
#include "alt_types.h"
#include "sys/alt_irq.h"
#include "altera_up_avalon_accelerometer_spi.h"
#include "altera_avalon_timer_regs.h"
#include "altera_avalon_timer.h"
#include "altera_avalon_pio_regs.h"
#include "altera_avalon_uart.h"
#include "altera_avalon_uart_regs.h"
#include "altera_avalon_uart_fd.h"

int timer = 0;
int transmission = 0;

void timer_init(void* isr) {
    alt_irq_register(TIMER_IRQ, 0, isr);
    IOWR_ALTERA_AVALON_TIMER_PERIODL(TIMER_BASE, 0x0900);
    IOWR_ALTERA_AVALON_TIMER_PERIODH(TIMER_BASE, 0x0090);
    IOWR_ALTERA_AVALON_TIMER_CONTROL(TIMER_BASE, 0x7);              // not sure why masks wont work unless in a specific oder
}

void timer_isr(void* context) {
    IOWR_ALTERA_AVALON_TIMER_STATUS(TIMER_BASE, 0x1);               // clear interrupt
    timer = 1;
}

void setup_button_interrupts(void* isr) {
    IOWR_ALTERA_AVALON_PIO_EDGE_CAP(BUTTON_BASE, 0xFF);             // clear capture register by writing 1
    IOWR_ALTERA_AVALON_PIO_IRQ_MASK(BUTTON_BASE, 0x3);              // enable interrupts for both KEY buttons
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

void process_buffer(char* text) {
    if (text == NULL) {
        alt_putstr("Error: Buffer is NULL.\n");
        return;
    }
    else if (strcmp(text, "start") == 0) {
        alt_putstr("START\n");
    }
    else if (strcmp(text, "stop") == 0) {
        alt_putstr("STOP\n");
    }
    else if (strcmp(text, "info") == 0) {
        alt_putstr("INFO\n");
    }
    else if (strcmp(text, "rate") == 0) {
        alt_putstr("RATE\n");
    }
    else {
    }
}

int main() {

    alt_32 x_read;
    alt_32 y_read;
    alt_32 z_read;

    char rd_buffer[256];
    char* message;

    alt_up_accelerometer_spi_dev* acc_dev = alt_up_accelerometer_spi_open_dev("/dev/accelerometer_spi");
    int uart_fd = open("/dev/uart", O_NONBLOCK | O_RDWR); // file descriptor (int) of uart core

    if (uart_fd == 0 || acc_dev == NULL) {
        alt_putstr("FAILURE: `/dev/accelerometer_spi` or `/dev/uart` failed to open.");
    } else {
        alt_putstr("SUCCESS: `/dev/accelerometer_spi` and `/dev/uart` opened successfully.\n");
    }

    timer_init(timer_isr);
    setup_button_interrupts(button_isr);

    while (1) {
        if(timer){

            ssize_t bytes_read = read(uart_fd, rd_buffer, sizeof(rd_buffer)-1);
            if(bytes_read > 0) {
                // alt_putstr(rd_buffer);
                process_buffer(rd_buffer);
            }

            if(transmission){

                alt_up_accelerometer_spi_read_x_axis(acc_dev, &x_read);
                alt_up_accelerometer_spi_read_y_axis(acc_dev, &y_read);
                alt_up_accelerometer_spi_read_z_axis(acc_dev, &z_read);
                
                sprintf(message, "%d, %d, %d\n", (int)x_read, (int)y_read, (int)z_read);
                write(uart_fd, message, strlen(message)); // write returns bytes written, useful for debugging.

                timer = 0;
            }
        }
    }

    return 0;
}