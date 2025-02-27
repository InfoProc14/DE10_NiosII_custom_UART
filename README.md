# Summary
- `src/` contains several decoupled examples on how to use different aspects of the UART core. Also includes the top level `.sv` file for the system.
    - Note that file descriptors (`int`) are used to target the UART core.
    - They can be initiated with non-blocking operation.
    - They use `open()`, `read()` and `write()`.
    - Descriptors give finer control than using file pointers (`FILE*`) but both are accessed via `"/dev/uart"`.

- The system `.qsys` and `.sopcinfo` are located in `platform-designer/`

- Current implementation of `software/uart_accelerometer/` attemps to combine transmission of accelerometer data via UART, with the capability to read and process incoming UART.
    - Printing incoming UART commented out bc its messy with transmit and receive in the same terminal.

---

# Implementation
## system specification
- Nios II/e with 128,800 bytes of on-chip memory
- Accelerometer SPI
- Intel UART (RS-232) Core
    - **Baud rate 115200**. 8 data bits, 1 stop bit. 
- Intel JTAG UART Core
- 1 Interval Timer
- DE10 Peripherals: buttons, switches and hex displays

The `RX` `TX` pins of the UART core are exposed connections and have been mapped to:
- UART core `RX` -> `D0` of Arduino GPIO
- UART core `TX` -> `D1` of Arduino GPIO

Note that `D0` and `D1` are the Arduino's UART pins for `TX` and `RX` respectively.

## Software
- Transmits accelerometer XYZ data via UART upon each timer interrupt
    - The timer ISR controls the global flag `timer`, setting it to 1
    - The main while loop enters upon this condition, and resets it to 0 after executing
    - The timer period can be changed to modify the sample rate of accelerometer data

- Global flag `transmission` used to enable/disable transmission
    - Accelerometer data will only transmit with `timer` and `transmission` enabled

- UART core opened as non-blocking, and attempts to read into buffer on every `timer` loop
    - Only process the buffer if > 0 bytes read successfully

- Buttons used to control `transmission` flag via interrupts
    - `KEY[0]` enables transmission
    - `KEY[1]` disables transmission

---

# Issues

- Ideally would like process the UART read independant of the `timer` flag (i.e. poll on every iteration of the main while loop)
    - However, as it has been initiated as non-blocking, the reads will be incomplete if the polling frequency is too high.
    - I.e. if reading `"Hello World\n"` it will generally only manage to read the first character "H" before the next read intercepts it. Results in something like `"HHHHHHHHHHHHHHH <...>"`

- Similarly, writing can overwrite the previous write before it is completed, given a high enough frequency.
    - If writing `"Hello World\n"`, it again may only manage to transmit "H" before the next write intercepts it. Again, results in transmitting something like `"HHHHHHHHHHHHHHH <...>"`

- No filter implemented yet. 
- Also a bunch of headers that I probably don't need.