#include "byteBuffer.h"
#include <winsock2.h>

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

bool ByteBuffer::Write(const void* inBytes, size_t count)
{
    CheckValid();

    // Writing 후 확인을 위해 현재 여유 공간 계산해둠.
    size_t curFreeSpace = GetBufferFreeSpace();
#ifdef _DEBUG
    size_t curReadableSpace = GetReadableSpace();
#endif
    size_t writtenBytes = 0;

    if (curFreeSpace < count)
    {
        return false;
    }

    ASSERT(bufferSize_ >= writePos_);
    size_t untillEnd = bufferSize_ - writePos_;
    const char* bytes = static_cast<const char*>(inBytes);
    if (untillEnd <= count)
    {
        // 끝까지 쓰고 writePos_ 을 0으로 옮긴다.
        if (untillEnd > 0)
        {
            memcpy(buffer_ + writePos_, bytes, untillEnd);
            bytes += untillEnd;
            count -= untillEnd;
            writtenBytes = untillEnd;
        }
        writePos_ = 0;
    }

    if (count > 0)
    {
        memcpy(buffer_ + writePos_, bytes, count);
        writePos_ += count;
        writtenBytes += count;
    }

    ASSERT(GetBufferFreeSpace() == curFreeSpace - writtenBytes);

#ifdef _DEBUG
    ASSERT(GetReadableSpace() == curReadableSpace + writtenBytes);
#endif
    return true;
}

bool ByteBuffer::CanReadBytes(size_t count) const
{
    CheckValid();
    return count <= GetReadableSpace();
}

bool ByteBuffer::Read(ContiguousByteContainer& out, size_t count)
{
    CheckValid();
    if (!CanReadBytes(count))
    {
        return false;
    }

    out.clear();
    out.reserve(count);
    ASSERT(bufferSize_ >= readPos_);
    size_t bytesToEndOfBuffer = bufferSize_ - readPos_;
    if (bytesToEndOfBuffer <= count)
    {
        // Ringbuffer 의 끝까지 읽고 readPos_ 을 0으로 옮긴다.
        if (bytesToEndOfBuffer > 0)
        {
            out.assign(buffer_ + readPos_, bytesToEndOfBuffer);
            ASSERT(count >= bytesToEndOfBuffer);
            count -= bytesToEndOfBuffer;
        }
        readPos_ = 0;
    }

    if (count > 0)
    {
        out.append(buffer_ + readPos_, count);
        readPos_ += count;
    }
    return true;
}

void ByteBuffer::ReadAll(ContiguousByteContainer& data)
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

    return true;
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

bool ByteBuffer::CanWriteBytes(size_t count) const
{
    CheckValid();
    return count <= GetBufferFreeSpace();
}

bool ByteBuffer::ReadBuffer(void* out, size_t count)
{
    CheckValid();
    if (!CanReadBytes(count))
    {
        return false;
    }

    ASSERT(bufferSize_ >= readPos_);

    char* dst = static_cast<char*>(out);
    size_t bytesToEndOfBuffer = bufferSize_ - readPos_;
    if (bytesToEndOfBuffer <= count)
    {
        // Ringbuffer 의 끝까지 읽고 readPos_ 을 0으로 옮긴다.
        if (bytesToEndOfBuffer > 0)
        {
            memcpy(dst, buffer_ + readPos_, bytesToEndOfBuffer);
            dst += bytesToEndOfBuffer;
            count -= bytesToEndOfBuffer;
        }
        readPos_ = 0;
    }

    if (count > 0)
    {
        memcpy(dst, buffer_ + readPos_, count);
        readPos_ += count;
    }
    return true;
}
