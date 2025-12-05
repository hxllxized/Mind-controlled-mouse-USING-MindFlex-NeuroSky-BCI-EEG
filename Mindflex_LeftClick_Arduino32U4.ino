#include <Mouse.h>

String incoming = "";
int attentionValue = 0;

void setup() {
  Serial.begin(115200);
  Mouse.begin();
}

void loop() {

  // ---- Read EEG attention values from Python ----
  if (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      if (incoming.startsWith("ATT:")) {
        attentionValue = incoming.substring(4).toInt();
      }

      // ---- Trigger left click on demand ----
      if (incoming.startsWith("CLICK")) {
        Mouse.click(MOUSE_LEFT);
        delay(200);
      }

      incoming = "";
    } else {
      incoming += c;
    }
  }
}
