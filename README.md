# Summary  
- `src/` contains several decoupled examples on how to use different aspects of the UART core. Also includes the top level `.sv` file for the system.
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

## System Specification  
- **Nios II/e CPU** with 128,800 bytes on-chip memory.
- **Accelerometer**: SPI interface for XYZ-axis data.
- **UART Cores**:  
    - Intel UART (RS-232) at **115200 baud**, 8 data bits, 1 stop bit.
    - JTAG UART for debugging.
- **Timers**:  
    - `TIMER_MAIN`: Controls accelerometer sampling rate (default 400 ms period).
    - `TIMER_POLL`: Polls UART for incoming data (default 10 ms period).
- **DE10 Peripherals**: Buttons, switches, and hex displays.
- **Pin Mapping**:  
    - UART `RX` → Arduino GPIO `D0`.
    - UART `TX` → Arduino GPIO `D1`.

---

## Software  

### Core Features  
1. **Filtered Accelerometer Data Transmission**:  
    - Uses a **FIR filter** with predefined coefficients (length `FILTER_N = 49`) to process raw accelerometer data.
    - Filter buffers store recent samples; convolution is applied during transmission.
    - Output format: `x_filtered y_filtered z_filtered\n`.

2. **UART Command Interface**:  
    - Processes commands from UART (e.g., `start`, `stop`, `rate 10`).
    - Supported commands:
        - `id`: Returns the current device ID based on the state of `SW[0]`.
        - `start`: Enable data transmission.
        - `stop`: Disable data transmission.
        - `rate <Hz>`: Set sampling rate (e.g., `rate 5` → 200 ms period).
        - `info`: Display system status (transmission state, timer period, baud rate).
        - `help`: List available commands.

3. **Interrupt-Driven Control**:  
    - **Buttons**:  
        - `KEY[0]`: Enable transmission (ISR sets `transmission = 1`).
        - `KEY[1]`: Disable transmission (ISR sets `transmission = 0`).
    - **Timers**:  
        - `TIMER_MAIN`: Triggers accelerometer sampling and filtering.
        - `TIMER_POLL`: Checks UART for incoming data.

4. **Device ID Management**:

    - The FPGA switch SW[0] controls the device ID.
        - SW[0] = 0 (pulled DOWN): `ID 1`.
        - SW[1] = 1 (pulled UP): `ID 2`.
        - The "`id`" UART command queries the current state of `SW[0]` and returns the corresponding device ID.

---

### Configuration  
- **Sampling Rate**: Adjust `timer_main_period_ms` (default 400 ms) or send `rate <Hz>` via UART.
- **Filter Coefficients**: Modify `filt_coeff` array in `filter_obj` for custom FIR behavior. 

--- 
