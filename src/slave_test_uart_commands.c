#include <Arduino.h>

#define FPGA_BAUD 115200
#define RXD2 16
#define TXD2 17

HardwareSerial FPGA_UART(2);

void send_wait_receive(const char* msg) {
  FPGA_UART.print(msg);
  // Serial.print(msg);
  delay(100);
  while (FPGA_UART.available() > 0) {
    char c = FPGA_UART.read();
    Serial.print(c);
  }
  delay(1000);
}

void setup() {

  Serial.begin(115200);
  FPGA_UART.begin(FPGA_BAUD, SERIAL_8N1, RXD2,TXD2);

}

void loop() {

  send_wait_receive("start\n");
  send_wait_receive("stop\n");
  send_wait_receive("info\n");
  send_wait_receive("rate 6\n");
  send_wait_receive("rate 0\n");
  send_wait_receive("help\n");

}