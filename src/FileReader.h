#pragma once
#include <vector>
#include <cstdint>
#include <stdexcept>
#include <string>

namespace shx
{


    class FileReader {
    public:
        explicit FileReader(const uint8_t* data, size_t size);
        explicit FileReader(const std::vector<uint8_t>& data);

        // Interprets an unsigned byte as a signed byte (two's complement).
        static int8_t byteToSByte(uint8_t value) noexcept
        {
            return static_cast<int8_t>(value);
        }

        std::vector<uint8_t> readBytes(size_t length);
        void skip(size_t length);
        uint8_t  readUint8();
        int8_t   readInt8();
        uint16_t readUint16(bool littleEndian = true);
        int16_t  readInt16();
        uint32_t readUint32();
        int32_t  readInt32();
        float    readFloat32();
        double   readFloat64();

        void   setPosition(size_t position);
        bool   isEnd() const noexcept;
        size_t currentPosition() const noexcept { return position_; }
        size_t length() const noexcept { return size_; }

    private:
        const uint8_t* data_;
        size_t size_;
        size_t position_{ 0 };

        void checkBounds(size_t required) const;
    };

}
