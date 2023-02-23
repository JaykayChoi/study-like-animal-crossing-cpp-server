#include "byteBuffer.h"
#include <winsock2.h>

#if (defined(__i386__) || defined(__alpha__) || defined(__ia64) || defined(__ia64__)     \
    || defined(_M_IX86) || defined(_M_IA64) || defined(_M_ALPHA) || defined(__amd64)     \
    || defined(__amd64__) || defined(_M_AMD64) || defined(__x86_64)                      \
    || defined(__x86_64__) || defined(_M_X64) || defined(__bfin__) || defined(__ARMEL__) \
    || (defined(_WIN32) && defined(__ARM__) && defined(_MSC_VER)))
#define IS_LITTLE_ENDIAN
#else
#define IS_BIG_ENDIAN
#endif

/////////////////////////////////////////////////////////////////////////////////////////
// ByteBuffer

ByteBuffer::ByteBuffer(size_t bufferSize)
    : buffer_(new std::byte[bufferSize + 1])
    , bufferSize_(bufferSize + 1)
    , dataStart_(0)
    , writePos_(0)
    , readPos_(0)
{
}

ByteBuffer::~ByteBuffer()
{
    CheckValid();
    delete[] buffer_;
    buffer_ = nullptr;
}

bool ByteBuffer::Write(const void* inBytes, size_t size)
{
    CheckValid();

    // Writing 후 확인을 위해 현재 여유 공간 계산해둠.
    size_t curFreeSpace = GetBufferFreeSpace();
#ifdef _DEBUG
    size_t curReadableSpace = GetReadableSpace();
#endif
    size_t writtenBytes = 0;

    if (curFreeSpace < size)
    {
        return false;
    }

    ASSERT(bufferSize_ >= writePos_);
    size_t untillEnd = bufferSize_ - writePos_;
    const char* bytes = static_cast<const char*>(inBytes);
    if (untillEnd <= size)
    {
        // 끝까지 쓰고 writePos_ 을 0으로 옮긴다.
        if (untillEnd > 0)
        {
            memcpy(buffer_ + writePos_, bytes, untillEnd);
            bytes += untillEnd;
            size -= untillEnd;
            writtenBytes = untillEnd;
        }
        writePos_ = 0;
    }

    if (size > 0)
    {
        memcpy(buffer_ + writePos_, bytes, size);
        writePos_ += size;
        writtenBytes += size;
    }

    ASSERT(GetBufferFreeSpace() == curFreeSpace - writtenBytes);

#ifdef _DEBUG
    ASSERT(GetReadableSpace() == curReadableSpace + writtenBytes);
#endif
    return true;
}

bool ByteBuffer::CanReadBytes(size_t size) const
{
    CheckValid();
    return size <= GetReadableSpace();
}

bool ByteBuffer::Read(std::basic_string<std::byte>& out, size_t size)
{
    CheckValid();
    if (!CanReadBytes(size))
    {
        return false;
    }

    out.clear();
    out.reserve(size);
    ASSERT(bufferSize_ >= readPos_);
    size_t bytesToEndOfBuffer = bufferSize_ - readPos_;
    if (bytesToEndOfBuffer <= size)
    {
        // Ringbuffer 의 끝까지 읽고 readPos_ 을 0으로 옮긴다.
        if (bytesToEndOfBuffer > 0)
        {
            out.assign(buffer_ + readPos_, bytesToEndOfBuffer);
            ASSERT(size >= bytesToEndOfBuffer);
            size -= bytesToEndOfBuffer;
        }
        readPos_ = 0;
    }

    if (size > 0)
    {
        out.append(buffer_ + readPos_, size);
        readPos_ += size;
    }
    return true;
}

void ByteBuffer::ReadAll(std::basic_string<std::byte>& data)
{
    CheckValid();
    Read(data, GetReadableSpace());
}

bool ByteBuffer::ReadToUInt8Arr(std::vector<uint8>& out, size_t numBytes)
{
    char buf[1024];

    while (numBytes != 0)
    {
        size_t num = (numBytes > sizeof(buf)) ? sizeof(buf) : numBytes;
        VERIFY(ReadBuffer(buf, num));
        out.assign(buf, buf + num);

        ASSERT(numBytes >= num);
        numBytes -= num;
    }
    return true;
}

bool ByteBuffer::ReadUInt8(uint8& out)
{
    CheckValid();
    if (!CanReadBytes(1))
    {
        return false;
    }

    ReadBuffer(&out, 1);
    return true;
}

bool ByteBuffer::ReadUInt32BE(uint32& out)
{
    CheckValid();
    if (!CanReadBytes(4))
    {
        return false;
    }

    ReadBuffer(&out, 4);
    out = ntohl(out);

    return true;
}

bool ByteBuffer::ReadUInt32LE(uint32& out)
{
    CheckValid();
    if (!CanReadBytes(4))
    {
        return false;
    }

    ReadBuffer(&out, 4);

#ifdef IS_BIG_ENDIAN
    out = ((out >> 24) & 0xff) | ((out >> 16) & 0xff00) | ((out >> 8) & 0xff0000)
        | (out & 0xff000000);
#endif

    return true;
}

bool ByteBuffer::ReadInt32LE(int& out)
{
    CheckValid();
    if (!CanReadBytes(4))
    {
        return false;
    }

    ReadBuffer(&out, 4);

#ifdef IS_BIG_ENDIAN
    out = ((out >> 24) & 0xff) | ((out >> 16) & 0xff00) | ((out >> 8) & 0xff0000)
        | (out & 0xff000000);
#endif

    return true;
}

bool ByteBuffer::WriteInt32LE(int value)
{
    CheckValid();
#ifdef IS_LITTLE_ENDIAN
    return Write((const char*)&value, 4);
#else
    int v = ((value >> 24) & 0xff) | ((value >> 16) & 0xff00) | ((value >> 8) & 0xff0000)
        | (value & 0xff000000);
    return Write((const char*)&v, 4);
#endif
}

void ByteBuffer::CommitRead()
{
    CheckValid();
    dataStart_ = readPos_;
}

void ByteBuffer::ResetRead()
{
    CheckValid();
    readPos_ = dataStart_;
}

void ByteBuffer::CheckValid() const
{
    ASSERT(readPos_ < bufferSize_);
    ASSERT(writePos_ < bufferSize_);
}

size_t ByteBuffer::GetBufferFreeSpace() const
{
    CheckValid();
    if (writePos_ >= dataStart_)
    {
        ASSERT(bufferSize_ >= writePos_);
        ASSERT((bufferSize_ - writePos_ + dataStart_) >= 1);
        return bufferSize_ - writePos_ + dataStart_ - 1;
    }

    ASSERT(bufferSize_ >= writePos_);
    ASSERT(bufferSize_ - writePos_ >= 1);
    return dataStart_ - writePos_ - 1;
}

size_t ByteBuffer::GetReadableSpace() const
{
    CheckValid();
    if (readPos_ > writePos_)
    {
        ASSERT(bufferSize_ >= readPos_);
        return bufferSize_ - readPos_ + writePos_;
    }

    ASSERT(writePos_ >= readPos_);
    return writePos_ - readPos_;
}

bool ByteBuffer::CanWriteBytes(size_t size) const
{
    CheckValid();
    return size <= GetBufferFreeSpace();
}

bool ByteBuffer::ReadBuffer(void* out, size_t size)
{
    CheckValid();
    if (!CanReadBytes(size))
    {
        return false;
    }

    ASSERT(bufferSize_ >= readPos_);

    char* dst = static_cast<char*>(out);
    size_t bytesToEndOfBuffer = bufferSize_ - readPos_;
    if (bytesToEndOfBuffer <= size)
    {
        // Ringbuffer 의 끝까지 읽고 readPos_ 을 0으로 옮긴다.
        if (bytesToEndOfBuffer > 0)
        {
            memcpy(dst, buffer_ + readPos_, bytesToEndOfBuffer);
            dst += bytesToEndOfBuffer;
            size -= bytesToEndOfBuffer;
        }
        readPos_ = 0;
    }

    if (size > 0)
    {
        memcpy(dst, buffer_ + readPos_, size);
        readPos_ += size;
    }
    return true;
}
