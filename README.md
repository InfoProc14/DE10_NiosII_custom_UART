# Summary
- `src/` contains several decoupled examples on how to use different aspects of the UART core. Also includes the top level `.sv` file for the system.
    - note that file descriptors (`int`) are used to target the UART core
    - they can be initiated with non-blocking operation.
    - they use `open()`, `read()` and `write()`
    - descriptors give finer control than using file pointers (`FILE*`)
    - but both are accessed via `"/dev/uart"`

- the system `.qsys` and `.sopcinfo` are located in `platform-designer/`

- current implementation of `software/uart_accelerometer/` attemps to combine transmission of accelerometer data via UART, with the capability to read and process incoming UART.
    - printing incoming UART commented out bc its messy with transmit and receive in the same terminal

- uuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuu