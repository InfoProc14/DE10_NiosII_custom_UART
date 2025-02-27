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

    alt_32 x_read;
    alt_32 y_read;
    alt_32 z_read;
    const char* message = "hello world\n";

    alt_up_accelerometer_spi_dev* acc_dev = alt_up_accelerometer_spi_open_dev("/dev/accelerometer_spi");
    int uart_fd = open("/dev/uart", O_NONBLOCK | O_RDWR);

    timer_init(timer_isr);

    while (1) {
        if(timer){ 
            int num = write(uart_fd, message, strlen(message));
            alt_putstr("faggot\n");
            timer = 0;
        }
    }

    return 0;
}