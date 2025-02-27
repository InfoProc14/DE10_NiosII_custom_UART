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

void timer_init(void* isr) {
    alt_irq_register(TIMER_IRQ, 0, isr);
    IOWR_ALTERA_AVALON_TIMER_PERIODL(TIMER_BASE, 0x0900);
    IOWR_ALTERA_AVALON_TIMER_PERIODH(TIMER_BASE, 0x0090);
    IOWR_ALTERA_AVALON_TIMER_CONTROL(TIMER_BASE, 0x7); // not sure why masks wont work unless in a specific oder
}

void timer_isr(void* context) {
    IOWR_ALTERA_AVALON_TIMER_STATUS(TIMER_BASE, 0x1); // clear interrupt
    timer = 1;
}

int main() {
    alt_putstr("PROGRAM STARTED\n");

    char buffer[256];
    int uart_fd = open("/dev/uart", O_NONBLOCK | O_RDWR); // file descriptor (int) of uart core

    timer_init(timer_isr);

    while (1) {
        if(timer) {
            ssize_t bytes = read(uart_fd, buffer, sizeof(buffer)-1);

            // note that read() returns # of bytes read, and returns -1 if invalid.
            if(bytes > 0) alt_putstr(buffer);

            timer = 0;
        }
    }
    
    return 0;
}


