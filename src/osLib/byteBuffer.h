#pragma once

#include "../global.h"

// Thread-unsafe ringbuffer.
class ByteBuffer
{
public:
    ByteBuffer(size_t bufferSize);
    ~ByteBuffer();

    // Ringbuffer 에 inBytes 를 쓴다. 성공 시 true 반환.
    bool Write(const void* inBytes, size_t count);

    // count 바이트를 읽을 수 있는 경우 true 반환.
    bool CanReadBytes(size_t count) const;

    // count 바이트를 읽는다. 성공하면 true 반환.
    bool Read(std::basic_string<std::byte>& out, size_t count);

    // 사용 가능한 모든 데이터를 읽는다.
    void ReadAll(std::basic_string<std::byte>& out);

    bool ReadToUInt8Arr(std::vector<uint8>& out, size_t numBytes);

    bool ReadUInt8(uint8& out);

    bool ReadUInt32BE(uint32& out);

    bool ReadUInt32LE(uint32& out);

    bool ReadInt32LE(int& out);

    // 읽은 바이트를 제거.
    void CommitRead();

    // 읽은 바이트 되돌리기.
    void ResetRead();

protected:
    std::byte* buffer_;
    size_t bufferSize_; // Total size.

    size_t dataStart_; // 데이터가 시작되는 위치.
    size_t writePos_; // 데이터가 끝나는 위치.
    size_t readPos_; // 다음 읽기가 시작되는 위치.

    // readPos_, writePos_ 검사.
    void CheckValid() const;

    // Ringbuffer 에 쓸 수 있는 바이트수 반환.
    size_t GetBufferFreeSpace() const;

    // 현재 읽을 수 있는 바이트수 반환.
    size_t GetReadableSpace() const;

    // count 바이트를 쓸 수 있는 경우 true 반환.
    bool CanWriteBytes(size_t count) const;

    // count 바이트를 buffer 로 읽는다. 성공하면 true 반환.
    bool ReadBuffer(void* out, size_t count);
};
