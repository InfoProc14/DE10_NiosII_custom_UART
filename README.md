# Summary  

- `src/` contains several decoupled examples on how to use different aspects of the UART core. Also includes the top-level `.sv` file for the system.
    - Uses file descriptors (`int`) for UART control (opened via `open("/dev/uart", ...)`).
    - Non-blocking UART operations enabled for concurrent transmit/receive.
    - File descriptors provide lower-level control compared to `FILE*` streams.

- System design files (`.qsys`, `.sopcinfo`) are in `platform-designer/`.

- `software/uart_accelerometer/` integrates accelerometer data filtering and UART communication:  
    - Transmits filtered XYZ accelerometer data at configurable intervals.
    - Processes incoming UART commands (e.g., `start`, `stop`, `rate`).
    - Device ID is controlled by FPGA switch `SW[0]` and can be queried via the `id` UART command.

---

# Implementation  

## FPGA Hardware Specification and Firmware  

The DE10 system is configured with the following hardware components:
- **Nios II/e CPU** with 128 KB On-Chip RAM.
- **On-Chip Flash ROM** (MAX10): Used for storing the Nios II system configuration and application program.
- **UART Cores**:  
    - RS-232 UART (115,200 baud, 8 data bits, 1 stop bit).
    - JTAG UART for debugging.
- **Timers**:  
    - Two interval timers:  
        - `TIMER_MAIN`: Controls accelerometer sampling rate (default 100 ms period).  
        - `TIMER_POLL`: Polls UART for incoming data (default 10 ms period).  
- **Peripherals**:  
    - PIO for push buttons (`KEY[1:0]`), switches (`SW[9:0]`), and hex displays.  
    - SPI Accelerometer interface for XYZ-axis data acquisition.  

### Compilation and Flashing Process  
- The system was compiled to a `.sof` bitstream, and the Nios II C program (compiled with the BSP) produced a `.hex` data file.  
- These two files were then converted into a single `.pof` file (using Quartus' "Convert Programming Files"), which was flashed onto the on-chip Flash memory.  
- This configures the DE10 to boot from the Nios II system stored in the **Configuration Flash Memory (CFM)** and execute the **boot-copier** upon system reset. The boot-copier copies the application from the **User Flash Memory (UFM)** sector to the on-chip RAM.  
- This internal process enables both the Nios II system and program to persist across power cycles.
- Reference for [Configuring Nios II Processor Booting Methods in MAX 10 FPGA Devices](https://www.eeweb.com/wp-content/uploads/articles-app-notes-files-nios-ii-processor-booting-methods-in-max-10-fpga-devices-1491981243.pdf).

---

## Software  

### Core Features  

1. **Filtered Accelerometer Data Transmission**:  
    - Uses a **49-tap low-pass FIR filter** with predefined coefficients to process raw accelerometer data.  
    - Filter buffers store recent samples; convolution is applied during transmission.  
    - Output format: `x_filtered y_filtered z_filtered\n`.  

2. **UART Command Interface**:  
    - Processes commands from UART (non-blocking polling at 10 ms intervals).  
    - Supported commands:  
        - `start`: Enable data transmission.  
        - `stop`: Disable data transmission.  
        - `rate <Hz>`: Set sampling rate by modifying the timer period (e.g., `rate 10` → 100 ms period).  
        - `info`: Display system status (baud rate, transmission state, sampling rate, node ID).  
        - `id`: Query the current node ID based on the state of `SW[0]`.  
        - `help`: List available commands.  

3. **Interrupt-Driven Control**:  
    - **Push Buttons (`KEY[1:0]`)**:  
        - `KEY[0]`: Enable data transmission (ISR sets `transmission = 1`).  
        - `KEY[1]`: Disable data transmission (ISR sets `transmission = 0`).  
    - **Timers**:  
        - `TIMER_MAIN`: Triggers accelerometer sampling and filtering (default 100 ms period).  
        - `TIMER_POLL`: Polls UART for incoming data (default 10 ms period).  

4. **Device ID Management**:  
    - The FPGA switch `SW[0]` controls the device node ID:  
        - `SW[0] = 0` (pulled DOWN): `ID 1` (bicep).  
        - `SW[0] = 1` (pulled UP): `ID 2` (forearm).  
    - The `id` UART command queries the current state of `SW[0]` and returns the corresponding device ID.  

---

### Configuration  

- **Sampling Rate**: Adjust `TIMER_MAIN` period (default 100 ms) or send `rate <Hz>` via UART.  
- **Filter Coefficients**: Modify the `filt_coeff` array in `filter_obj` for custom FIR behavior.  

---
