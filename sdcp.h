#include "LoopbackStream.h"
#include "Arduino.h"

#ifndef _SDCP_H_
#define _SDCP_H_

#define SDCP_HEADER_SIZE 10
#define SDCP_RLT_SPEED 80 // 10 bytes per second
#define SDCP_LOW_SPEED 800 // 100 bytes per second
#define SDCP_MEDIUM_SPEED 8000 // 1000 bytes per second (1 kbps)
#define SDCP_DEFAULT_SPEED 8000 // 1000 bytes per second (1 kbps)
#define SDCP_HIGH_SPEED 16000 // 2000 bytes per second (2 kbps)
#define SDCP_MAX_SPEED 32000 // 4000 bytes per second (4 kpbs), this speed is the maximum speed at which the protocol works fine (tested)
// the speed is limited by the processing time behind this procotol on a hardware similar to the Arduino Uno

#define SDCP_PACKET_DISCOVER 0
#define SDCP_PACKET_DEVICE_INFO 1
#define SDCP_PACKET_SET_MODE 2
#define SDCP_PACKET_SWITCH_MODE 3
#define SDCP_PACKET_LINE_PAUSE 4
#define SDCP_PACKET_LINE_TEST 5
#define SDCP_PACKET_LINE_OK 6
#define SDCP_PACKET_TRANSACTION_START 7
#define SDCP_PACKET_TRANSACTION_END 8
#define SDCP_PACKET_DISCONNECT 9

#define SDCP_STATE_NOT_ENOUGH 0
#define SDCP_STATE_INVALID_PACKET 1
#define SDCP_STATE_PACKET_READY 2
#define SDCP_STATE_PACKET_SENT 3
#define SDCP_STATE_READ_TIMEOUT 4

class SDCP {

public:
    SDCP(uint8_t deviceId, const String& deviceName, int clockPin, int lanePins[], int lanes, int _holdPin);
    void begin(bool master); // Master / Slave device setup
    uint8_t sendAndReceive(LoopbackStream *buf, int speed, int timeout);
    void send(LoopbackStream *buf, int speed);
    void sendDebug(uint8_t target, String out);
    void trigger();
    bool available();
    void read();
    void doDiscover();
    uint8_t getMasterId();
    String getMasterName();

private:
    void sendByte(byte b, unsigned int lanes, unsigned int dT);
    void receive();
    void processPacket();
    int decodePacket();
    
    void sendDeviceInfo();
    void reset();

    String _device_name;
    String _master_name;
    uint8_t _device_id;
    uint8_t _master_id;
    int _clockPin;
    int _holdPin;
    unsigned int _lane_pins_size;
    unsigned int _active_lanes;
    int* _lane_pins;
    volatile uint8_t byteBuf = 0;
    volatile int counter = 0;
    LoopbackStream readBuffer;
    LoopbackStream writeBuffer;
    int storedPacketId = -1;
    volatile bool _busy;
    volatile bool _available;
};

#endif //_SDCP_H_
