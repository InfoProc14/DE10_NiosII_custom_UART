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

#define UART_BUFFER_LIM 64
#define FILTER_N 49

int poll_uart = 0;
int timer_main = 0;
int transmission = 0;

alt_32 timer_poll_period_ms = 10;
alt_32 timer_main_period_ms = 400;

typedef struct filter_s {
    alt_32 x_buffer[FILTER_N];
    alt_32 y_buffer[FILTER_N];
    alt_32 z_buffer[FILTER_N];
    alt_16 filt_coeff[FILTER_N];
    int buffer_offset;
} filter_s;

void push_to_buffers(filter_s* filter, alt_32 x, alt_32 y, alt_32 z){
    filter->x_buffer[filter->buffer_offset] = x;
    filter->y_buffer[filter->buffer_offset] = y;
    filter->z_buffer[filter->buffer_offset] = z;
    filter->buffer_offset = (filter->buffer_offset+1) % FILTER_N;
}

void convolve(const filter_s* filter, alt_32* x, alt_32* y, alt_32* z){
    alt_32 x_sum = 0;
    alt_32 y_sum = 0;
    alt_32 z_sum = 0;

    for(int i = 0; i < FILTER_N; i++) {
		int buffer_index = (i+filter->buffer_offset-1) % FILTER_N;
		int coeff_index = FILTER_N - i;
		x_sum += filter->x_buffer[buffer_index] * (alt_32)filter->filt_coeff[coeff_index];
		y_sum += filter->y_buffer[buffer_index] * (alt_32)filter->filt_coeff[coeff_index];
		z_sum += filter->z_buffer[buffer_index] * (alt_32)filter->filt_coeff[coeff_index];
    }

    *x = x_sum >> 16;
    *y = y_sum >> 16;
    *z = z_sum >> 16;
}

void init_timer(alt_64 base, alt_64 base_irq, alt_32 period_ms, void* isr) {
    alt_irq_register(base_irq, 0, isr);

    unsigned int clock_frequency = 50000000; // 50 MHz
    unsigned int countdown_value = (clock_frequency / 1000) * period_ms;

    IOWR_ALTERA_AVALON_TIMER_PERIODL(base, countdown_value & 0xFFFF);
    IOWR_ALTERA_AVALON_TIMER_PERIODH(base, (countdown_value >> 16) & 0xFFFF);

    IOWR_ALTERA_AVALON_TIMER_CONTROL(base, 0x7); // not sure why masks wont work unless in a specific oder
}

void timer_main_isr(void* context) {
    IOWR_ALTERA_AVALON_TIMER_STATUS(TIMER_MAIN_BASE, 0x1);          // clear interrupt
    timer_main = 1;
}

void timer_poll_isr(void* context) {
    IOWR_ALTERA_AVALON_TIMER_STATUS(TIMER_POLL_BASE, 0x1);          // clear interrupt
    poll_uart = 1;
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

void uart_wait_trdy() {
    while ((IORD_ALTERA_AVALON_UART_STATUS(UART_BASE) & ALTERA_AVALON_UART_STATUS_TRDY_MSK) == 0) {
        alt_putstr("");
    }
}

void process_buffer(char* text, int uart_fd) {
    char message[UART_BUFFER_LIM];
    if (text == NULL) {
        sprintf(message, "[UART]: ERROR Buffer is NULL.\n");
        write(uart_fd, message, strlen(message));
        return;
    }
    else if (strstr(text, "start") != NULL) { // note currently constructed with if-else, so string with multiple commands will only process the first one
        sprintf(message, "[UART]: Detected `start` command. Enabling data transmission.\n");
        write(uart_fd, message, strlen(message));
        transmission = 1;
    }
    else if (strstr(text, "stop") != NULL) {
        sprintf(message, "[UART]: Detected `stop` command. Disabling data transmission.\n");
        write(uart_fd, message, strlen(message));
        transmission = 0;
    }
    else if (strstr(text, "info") != NULL) {
        sprintf(message, "[UART]: Detected `info` command. ");
        write(uart_fd, message, strlen(message));
        uart_wait_trdy();
        sprintf(message, "TRANSMISSION STATUS: %d | TIMER PERIOD: %d ms | ", transmission, timer_main_period_ms);
        write(uart_fd, message, strlen(message));
        uart_wait_trdy();
        sprintf(message, "BAUD RATE: %d | CPU FREQ: %d Hz\n", UART_BAUD, NIOS2_CPU_FREQ);
        write(uart_fd, message, strlen(message));

        // sprintf(message, "[UART]: Detected `info` command. TRANSMISSION STATUS: %d | TIMER PERIOD: %d ms | BAUD RATE: %d | CPU FREQ: %d hz\n", transmission, timer_main_period_ms, UART_BAUD, NIOS2_CPU_FREQ);
    }
    else if (strstr(text, "rate") != NULL) {
        sprintf(message, "[UART]: Detected `rate` command. ");
        write(uart_fd, message, strlen(message));
        uart_wait_trdy();

        int rate = 0;
        int matched = sscanf(text, "rate %d", &rate);
        if (matched && rate != 0) {
            sprintf(message, "Accelerometer data sample rate set to %d Hz.\n", rate);
            write(uart_fd, message, strlen(message));
    
            timer_main_period_ms = 1000 / rate;
        }
        else {
            sprintf(message, "Unable to parse a valid sample rate. ");
            write(uart_fd, message, strlen(message));
            uart_wait_trdy();
            sprintf(message, "Please format as `rate <number>` in Hz.\n");
            write(uart_fd, message, strlen(message));
        }
    }
    else if (strstr(text, "help") != NULL) {
        sprintf(message, "[UART]: Detected `help` command. ");
        write(uart_fd, message, strlen(message));
        uart_wait_trdy();
        sprintf(message, "Commands: `start`, `stop`, `info`, `rate <number>`, `help`.\n");
        write(uart_fd, message, strlen(message));
    }
    else {
    }
    // alt_putstr(message);
}

int main() {

    alt_32 x_read, x_read_filtered;
    alt_32 y_read, y_read_filtered;
    alt_32 z_read, z_read_filtered;

    filter_s filter_obj = {
        .x_buffer = {0},
        .y_buffer = {0},
        .z_buffer = {0},
        .buffer_offset = 0,
        .filt_coeff = {0b1111111111000100, 0b1111111101001100, 0b1111111101011001, 0b0000000011101001, 0b0000001101111000, 0b0000010001101111, 0b0000000111111001, 0b1111111001010110, 0b1111111000001001, 0b0000000110001111, 0b0000001110001110, 0b0000000000011010, 0b1111101110101110, 0b1111110110110111, 0b0000010001110101, 0b0000010101010000, 0b1111110011011100, 0b1111011100111100, 0b1111111110111010, 0b0000110000111011, 0b0000011100000011, 0b1111000011001110, 0b1110101011101101, 0b0001000100110001, 0b0100111101011110, 0b0110111000011010, 0b0100111101011110, 0b0001000100110001, 0b1110101011101101, 0b1111000011001110, 0b0000011100000011, 0b0000110000111011, 0b1111111110111010, 0b1111011100111100, 0b1111110011011100, 0b0000010101010000, 0b0000010001110101, 0b1111110110110111, 0b1111101110101110, 0b0000000000011010, 0b0000001110001110, 0b0000000110001111, 0b1111111000001001, 0b1111111001010110, 0b0000000111111001, 0b0000010001101111, 0b0000001101111000, 0b0000000011101001, 0b1111111101011001, 0b1111111101001100, 0b1111111111000100}
    };

    char rd_buffer[UART_BUFFER_LIM];
    char wr_buffer[UART_BUFFER_LIM];

    alt_up_accelerometer_spi_dev* acc_dev = alt_up_accelerometer_spi_open_dev("/dev/accelerometer_spi");
    int uart_fd = open("/dev/uart", O_NONBLOCK | O_RDWR); // file descriptor (int) of uart core

    if (uart_fd == 0 || acc_dev == NULL) {
        alt_putstr("FAILURE: `/dev/accelerometer_spi` or `/dev/uart` failed to open.");
    } else {
        alt_putstr("SUCCESS: `/dev/accelerometer_spi` and `/dev/uart` opened successfully.\n");
    }

    init_timer(TIMER_MAIN_BASE, TIMER_MAIN_IRQ, timer_main_period_ms, timer_main_isr);
    init_timer(TIMER_POLL_BASE, TIMER_POLL_IRQ, timer_poll_period_ms, timer_poll_isr);
    setup_button_interrupts(button_isr);

    while (1) {
            if(poll_uart) {
                ssize_t bytes_read = read(uart_fd, rd_buffer, sizeof(rd_buffer)-1);
                if(bytes_read > 0) {
                    process_buffer(rd_buffer, uart_fd);
                }
                poll_uart = 0;
            }

            alt_up_accelerometer_spi_read_x_axis(acc_dev, &x_read);
            alt_up_accelerometer_spi_read_y_axis(acc_dev, &y_read);
            alt_up_accelerometer_spi_read_z_axis(acc_dev, &z_read);

            push_to_buffers(&filter_obj, x_read, y_read, z_read);

            if(timer_main && transmission){

                convolve(&filter_obj, &x_read_filtered, &y_read_filtered, &z_read_filtered);
                
                sprintf(wr_buffer, "%d, %d, %d\n", (int)x_read_filtered, (int)y_read_filtered, (int)z_read_filtered);

                write(uart_fd, wr_buffer, strlen(wr_buffer));
                // alt_putstr(wr_buffer);

                timer_main = 0;
            }        
    }

    return 0;
}