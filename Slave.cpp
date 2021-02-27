#include "SDCP.h"

volatile static bool next = false;
SDCP *instance;

SDCP::SDCP(uint8_t deviceId, const String &deviceName, int clockPin, int *lanePins, int lanes, int holdPin) {
    _device_id = deviceId;
    _device_name = deviceName;
    _clockPin = clockPin;
    _lane_pins = lanePins;
    _lane_pins_size = lanes;
    _active_lanes = 1;
    _holdPin = holdPin;
    instance = this;
    LoopbackStream buffer(128);
    readBuffer = buffer;
    LoopbackStream b(128);
    writeBuffer = b;
}

uint8_t SDCP::sendAndReceive(LoopbackStream* buf, int speed, int timeout){
    this->send(buf, speed);
    unsigned long start = millis();
    while(true){
        if(this->available()){
            this->read();
            break;
        }
        if(timeout != -1 && millis() - start > timeout){
            return SDCP_STATE_READ_TIMEOUT;
        }
    }
    return SDCP_STATE_PACKET_SENT;
}

void SDCP::send(LoopbackStream* buf, int speed){
    _busy = true;
    pinMode(_holdPin, OUTPUT);
    digitalWrite(_holdPin, HIGH);
    bool rise = false;
    pinMode(_clockPin, INPUT);
    while(true){
        bool c = digitalRead(_clockPin);
        if(rise && !c){
            break;
        }
        if(c == true && !rise){
            rise = true;
            continue;
        }
    }
    delay(100);
    pinMode(_clockPin, OUTPUT);
    for (unsigned int i = 0; i < _lane_pins_size; i++) {
        pinMode(_lane_pins[i], OUTPUT);
    }
    unsigned int available = buf->available();
    unsigned int dT = (1000000 / speed) / 2; // Max supported speed: 460000 bps (460 kbps)
    for (unsigned int i = 0; i < available; i++) {
        uint8_t c = buf->read();
        sendByte(c, _active_lanes, dT);
    }
    digitalWrite(_clockPin, LOW);
    pinMode(_clockPin, INPUT);
    for (unsigned int i = 0; i < _lane_pins_size; i++) {
        int lanePin = _lane_pins[i];
        digitalWrite(lanePin, LOW);
        pinMode(lanePin, INPUT);
    }
    digitalWrite(_holdPin, LOW);
    pinMode(_holdPin, INPUT);
    _busy = false;
}

void SDCP::receive(){
    bool clock = digitalRead(_clockPin);
    if (clock && next) {
        for (int i = 0; i < _active_lanes; i++) {
            int lanePin = _lane_pins[i];
            bool b = digitalRead(lanePin);
            if (b) {
                byteBuf |= (0x80 >> counter);
            }
            counter++;
        }

        if (counter == 8) {
            readBuffer.write(byteBuf);
            counter = 0;
            byteBuf = 0;
        }
        next = false;
    } else if(!clock) {
        next = true;
    }
}

void SDCP::sendByte(byte b, unsigned int lanes, unsigned int dT) {
    unsigned int iterations = 8 / lanes; // Max supported lanes at once: 8 lanes (8 bits or 1 byte per clock cycle)
    for (unsigned int i = 0; i < iterations; i++) {
        for (unsigned int j = 0; j < lanes; j++) {
            int lanePin = _lane_pins[j];
            bool bitState = b & (0x80 >> (i + j));
            digitalWrite(lanePin, bitState);
        }

        delayMicroseconds(dT);
        digitalWrite(_clockPin, HIGH);
        delayMicroseconds(dT);
        digitalWrite(_clockPin, LOW);
    }
}

int SDCP::decodePacket() {
    int available = readBuffer.available();
    if(available >= 6){
        readBuffer.reset(available);
        uint8_t targetId = readBuffer.read();
        int packetLength = readBuffer.readInt();
        int targetChecksum = readBuffer.read();
        int checksum = targetId + packetLength;
        checksum &= 0xFF;

        readBuffer.reset(available);
        for(int i = 0; i < available; i++){
            Serial.print(readBuffer.read(), HEX);
            Serial.print(" ");
        }
        readBuffer.reset(available);
        readBuffer.skip(6);

        if (targetChecksum == checksum) {
            if (targetId == _device_id && packetLength > 0) {
                if(readBuffer.available() >= packetLength){
                    const int id = readBuffer.readInt();
                    storedPacketId = id;
                    return SDCP_STATE_PACKET_READY;
                } else {
                    return SDCP_STATE_NOT_ENOUGH;
                }
            }
        }
        return SDCP_STATE_INVALID_PACKET;
    }
    return SDCP_STATE_NOT_ENOUGH;
}

void SDCP::processPacket() {
    const int id = storedPacketId;
    if(id == SDCP_PACKET_DISCOVER){
        _master_id = readBuffer.read();
        _master_name = readBuffer.readUTF();
        Serial.print("Master ID: ");
        Serial.println(_master_id);
        Serial.print("Master name: ");
        Serial.println(_master_name);
        this->sendDeviceInfo();
    } else if(id == SDCP_PACKET_DEVICE_INFO){
        int deviceId = readBuffer.read();
        String deviceName = readBuffer.readUTF();
        int lanes = readBuffer.readInt();
        int encryption = readBuffer.read();
        Serial.print("Device ID: ");
        Serial.println(deviceId);
        Serial.print("Device name: ");
        Serial.println(deviceName);
        Serial.print("Lanes: ");
        Serial.println(lanes);
        Serial.print("Flags: ");
        Serial.println(encryption, 16);
        this->sendDebug(deviceId, "Registration successful");
    } else if(id == 10){
        String out = readBuffer.readUTF();
        Serial.print("[Remote]> ");
        Serial.println(out);
    }
}

void SDCP::reset() {
    readBuffer.reset(0);
    storedPacketId = -1;
    _busy = false;
    next = true;
    _available = false;
}

void SDCP::sendDebug(uint8_t target, String out){
    writeBuffer.reset(0);
    size_t len = 0;
    len += writeBuffer.write(target); // Target
    writeBuffer.prepareHeader(); // Setups an empty header (5 bytes)
    len += writeBuffer.writeInt(10); // Packet ID
    len += writeBuffer.writeString(out);
    len += writeBuffer.writeHeader(/*4 + 4 + out.length()*/); // Fills up the header
    writeBuffer.reset(len);

    this->send(&writeBuffer, SDCP_DEFAULT_SPEED);
}

void SDCP::sendDeviceInfo() {
    Serial.println("Sending device info");
    writeBuffer.reset(0);
    size_t len = writeBuffer.write(_master_id); // Target
    writeBuffer.prepareHeader(); // Setups an empty header (6 bytes)
    len += writeBuffer.writeInt(SDCP_PACKET_DEVICE_INFO); // Packet ID
    // Payload
    len += writeBuffer.write(_device_id); // Device ID
    len += writeBuffer.writeString(_device_name); // Device Name
    len += writeBuffer.writeInt(_lane_pins_size); // Lanes
    len += writeBuffer.write(0); // Encryption field

    len += writeBuffer.writeHeader(); // Fills up the header
    writeBuffer.reset(len);
    this->sendAndReceive(&writeBuffer, SDCP_DEFAULT_SPEED, 10000);
}

void SDCP::trigger(){
    if(this->_busy) return;
    this->_available = true;
}

void SDCP::read(){
    if(!_available || _busy) return;
    //unsigned long start = millis();
    _busy = true;
    _available = false;
    Serial.print("Receiving...");
    pinMode(_clockPin, OUTPUT);
    digitalWrite(_clockPin, HIGH);
    delay(1);
    digitalWrite(_clockPin, LOW);
    pinMode(_clockPin, INPUT);
    
    while(digitalRead(_holdPin)){
        this->receive();
    }
    //unsigned long time = millis() - start;
    //Serial.print(time);
    //Serial.print("ms");

    int result = this->decodePacket();
    Serial.println("<" + String(result) + ">");
    if(result == SDCP_STATE_PACKET_READY){
        processPacket();
    }

    this->reset();
    _busy = false;
}

bool SDCP::available(){
    return _available;
}

static void messageInterrupt(){
    instance->trigger();
}

String SDCP::getMasterName(){
    return _master_name;
}

uint8_t SDCP::getMasterId(){
    return _master_id;
}

void SDCP::begin(bool master) {
    readBuffer.reset(0);
    _active_lanes = _lane_pins_size;
    pinMode(_holdPin, INPUT);
    pinMode(_clockPin, INPUT);
    for (int i = 0; i < _lane_pins_size; i++) {
        int lanePin = _lane_pins[i];
        digitalWrite(lanePin, LOW);
        pinMode(lanePin, INPUT);
    }
    _busy = false;
    attachInterrupt(digitalPinToInterrupt(_holdPin), messageInterrupt, RISING);
    if (master) {
        size_t len = 0;
        writeBuffer.reset(0);
        len += writeBuffer.write(0x23); // Target
        writeBuffer.prepareHeader(); // Setups an empty header (5 bytes)
        len += writeBuffer.writeInt(SDCP_PACKET_DISCOVER); // Packet ID
        len += writeBuffer.write(this->_device_id); // Payload
        len += writeBuffer.writeString(this->_device_name);
        len += writeBuffer.writeHeader(/*1 + 4 + this->_device_name.length() + 4*/); // Fills up the header
        writeBuffer.reset(len);

        this->sendAndReceive(&writeBuffer, SDCP_DEFAULT_SPEED, -1);
    }
}
void SDCP::doDiscover() {
    size_t len = 0;
    writeBuffer.reset(0);
    len += writeBuffer.write(0x23); // Target
    writeBuffer.prepareHeader(); // Setups an empty header (5 bytes)
    len += writeBuffer.writeInt(SDCP_PACKET_DISCOVER); // Packet ID
    len += writeBuffer.write(this->_device_id); // Payload
    len += writeBuffer.writeString(this->_device_name);
    len += writeBuffer.writeHeader(/*1 + 4 + this->_device_name.length() + 4*/); // Fills up the header
    writeBuffer.reset(len);
    this->sendAndReceive(&writeBuffer, SDCP_DEFAULT_SPEED, 100);
}