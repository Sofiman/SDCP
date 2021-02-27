#include <Arduino.h>
#include "LoopbackStream.h"

LoopbackStream::LoopbackStream(uint16_t buffer_size) {
    this->buffer = (uint8_t *) malloc(buffer_size);
    this->buffer_size = buffer_size;
    this->pos = 0;
    this->size = 0;
    this->savedPos = -1;
    this->setTimeout(0);
}

LoopbackStream::~LoopbackStream() {
    this->size = -1;
    free(buffer);
}

void LoopbackStream::reset(int head) {
    this->pos = 0;
    this->size = head;
    this->savedPos = -1;
}

int LoopbackStream::peek(){
    return buffer[pos];
}

int LoopbackStream::read() {
    if (size <= 0) {
        return -1;
    } else {
        int ret = buffer[pos];
        pos++;
        size--;
        if (pos >= buffer_size) {
            pos = 0;
        }
        return ret;
    }
}

size_t LoopbackStream::write(uint8_t v) {
    if (size == buffer_size) {
        return 0;
    } else {
        if (size >= buffer_size) {
            return 0;
        }
        buffer[pos] = v;
        pos++;
        size++;
        return 1;
    }
}

int LoopbackStream::available() {
    return size;
}

/*int LoopbackStream::availableForWrite() {
    return buffer_size - size;
}

int LoopbackStream::rewind(int p) {
    pos -= p;
    size += p;
    if (pos <= 0) {
        pos = buffer_size;
    }
    return pos;
}*/

int LoopbackStream::skip(int p) {
    pos += p;
    size -= p;
    if (pos >= buffer_size) {
        pos = 0;
    }
    return pos;
}

int LoopbackStream::readInt() {
    int num = 0;
    for(int i = 0; i < 4; i++){
        num <<= 8;
        num |= this->read();
    }

    return num;
}

size_t LoopbackStream::writeInt(int l) {
    unsigned int p = pos;
    if (size >= buffer_size-4) { // Check for buffer overflow
        return 0;
    }
    buffer[p] =     (uint8_t) ((l & 0xFF000000) >> 24 );
    buffer[p + 1] = (uint8_t) ((l & 0x00FF0000) >> 16 );
    buffer[p + 2] = (uint8_t) ((l & 0x0000FF00) >> 8  );
    buffer[p + 3] = (uint8_t) ((l & 0X000000FF)       );
    size += 4;
    pos += 4;
    return 4;
}

size_t LoopbackStream::writeString(String s){
    const int l = s.length();
    int written = this->writeInt(l);
    if(written == 0) return 0; // Check if the length was correctly written
    if (size >= buffer_size - l) { // Check for buffer overflow
        return 0;
    }
    // Write each character one by one
    for (int i = 0; i < l; ++i) {
        buffer[pos + i] = s[i];
    }
    size += l;
    pos += l;
    return l + 4;
}

String LoopbackStream::readUTF() {
    auto l = (unsigned int)this->readInt(); // Reads the string length saved previously
    if(l < 1){
        return "";
    }
    String ret = "";
    char c;
    for (unsigned int i = 0; i < l; ++i) { // Reads each character one by one
        c = (char) read();
        ret += c;
    }
    ret += '\0';
    return ret;
}

void LoopbackStream::prepareHeader() {
    savedPos = pos - 1;
    this->writeInt(0);
    this->write(0);
}

size_t LoopbackStream::writeHeader() {
    int previousPos = pos, previousSize = size; // Save current pos
    pos = 1;
    size = 0;

    writeInt(previousSize - 6);

    int checksum = 0; // Calculate header checksum to ensure that the header was correctly received
    // and the packet length is not corrupted (this can lead to offset errors)
    for(int i = 0; i < 5; i++){
        checksum += buffer[i];
    }
    checksum &= 0xFF;
    write(checksum);

    pos = previousPos;
    size = previousSize;
    return 5;
}

void LoopbackStream::overrideTarget(uint8_t target){
    if(savedPos < 0) return;
    int previousPos = pos + size, previousSize = size; // Save current pos
    pos = savedPos - 1;
    size = 0;
    write(target);
    pos = previousPos;
    size = previousSize;
}