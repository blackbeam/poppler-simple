#ifndef __MEMORY_STREAM
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __linux
#define SIZE_TYPE size_t
#define SSIZE_TYPE ssize_t
#define OFFSET_TYPE off64_t
#define SEEK_RETURN_TYPE int
#elif __APPLE__
#define SIZE_TYPE int
#define SSIZE_TYPE int
#define OFFSET_TYPE fpos_t
#define SEEK_RETURN_TYPE fpos_t
#endif

class MemoryStream;

class Cookie
{
public:
    Cookie(MemoryStream* stream) : stream(stream) {};

    ~Cookie() {};

    MemoryStream* getStream() { return stream; };

    SSIZE_TYPE write(const char *buf, SIZE_TYPE size);
    int close();

private:
    MemoryStream* stream;
};

class MemoryStream
{
public:
    MemoryStream() : buffer_given(false), offset(0), buffer(NULL), buffer_len(0) {
        cookie = new Cookie(this);
    };

    ~MemoryStream() {
        if (buffer != NULL && !buffer_given) delete buffer;
        delete cookie;
    };

    FILE* open();
    OFFSET_TYPE getBufferLen() { return offset; };
    char* giveBuffer() {
        buffer_given = true;
        return buffer;
    }

    SSIZE_TYPE write(const char *buf, SIZE_TYPE size);
    int close();

private:
    bool buffer_given;
    OFFSET_TYPE offset;
    char* buffer;
    OFFSET_TYPE buffer_len;
    Cookie* cookie;
};
#endif
