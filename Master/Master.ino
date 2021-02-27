#include <LoopbackStream.h>
#include <SDCP.h>

const byte deviceId = 0;
const String deviceName = "Arduino Uno (M)";
const int availableLanes = 1;
const int lanes[] = {4,5};
SDCP sdcp(deviceId, deviceName, 2, lanes, availableLanes, 3);

void setup() {
    digitalWrite(13, LOW);
    // put your setup code here, to run once:
    Serial.begin(9600);
    Serial.println("Starting discover sequence");
    sdcp.begin(true);
}

void loop() {
    if(sdcp.available()){
        sdcp.read();
    }
    if(Serial.available()){
        String in = Serial.readString();
        if(in == "/discover"){
            sdcp.doDiscover();
        } else {
            sdcp.sendDebug(0x23, in.substring(0, in.length() - 1));
        }
    }
}