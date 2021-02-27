#include <LoopbackStream.h>
#include <sdcp.h>

const byte deviceId = 0x23;
const String deviceName = "Arduino Uno (S)";
const int availableLanes = 1;
const int lanes[] = {4,5};
SDCP sdcp(deviceId, deviceName, 2, lanes, availableLanes, 3);

void setup() {
    Serial.begin(9600);
    Serial.println("Listenning for input");
    sdcp.begin(false);
}

void loop() {
  if(sdcp.available()){
    sdcp.read();
  }
  if(Serial.available()){
    String in = Serial.readString();
    sdcp.sendDebug(0, in.substring(0, in.length() - 1));
  }
}
