void setup() {
  Serial.begin(115200);
  Serial.println("Relay Driver");
  for (uint8_t i = 0; i < 8; i += 1)
    pinMode(2 + i, OUTPUT);
}

static int8_t chan = -1;
void loop() {
  if (Serial.available()) {
    char c = Serial.read();
    if (chan < 0) {
      if ((c >= '0') && (c <= '8'))
        chan = c - '0';
    }
    else {
      if ((c >= '0') && (c <= '1')) {
        bool newstate = c - '0';
        Serial.print(chan);
        Serial.print(' ');
        Serial.println(newstate);
        if (8 == chan) {
          for (uint8_t i = 0; i < 8; i += 1)
            digitalWrite(2 + i, newstate);
        }
        else
          digitalWrite(2 + chan, newstate);
      }
      Serial.print("*");
      chan = -1;
    }
  }
}
