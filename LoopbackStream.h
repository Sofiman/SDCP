#pragma once

#include <Stream.h>

/*
 * A LoopbackStream stores all data written in an internal buffer and returns it back when the stream is read.
 * 
 * If the buffer overflows, the last bytes written are lost.
 * 
 * It can be used as a buffering layer between components.
 */
class LoopbackStream : public Stream {
    uint8_t *buffer;
    uint16_t buffer_size;
    uint16_t pos, size;
    int savedPos;
public:
    static const uint16_t DEFAULT_SIZE = 64;

    explicit LoopbackStream(uint16_t buffer_size = LoopbackStream::DEFAULT_SIZE);

    ~LoopbackStream();

    virtual size_t write(uint8_t);
    //virtual int availableForWrite();

    virtual int available();

    virtual int read();

    virtual int peek();

    //virtual void flush();
    virtual int skip(int bytes);

    //virtual int rewind(int bytes);
    virtual void reset(int head);
    //virtual void mark();
    //virtual void unmark();

    // Data operations
    virtual size_t writeHeader();

    virtual void prepareHeader();

    virtual size_t writeInt(int l);

    virtual int readInt();

    virtual size_t writeString(String s);

    virtual String readUTF();

    virtual void overrideTarget(uint8_t target);
};