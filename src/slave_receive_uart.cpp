void setup() {
    Serial.begin(115200);
    Serial.println("Ready to receive data.");
  }
  
  void loop() {
    if (Serial.available() > 0) {
      char receivedChar = Serial.read();
      Serial.print(receivedChar);
    }
  }