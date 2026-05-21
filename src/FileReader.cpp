#include "FileReader.h"
#include <stdexcept>
#include <string>
#include <cstring>

namespace shx
{

    FileReader::FileReader(const uint8_t* data, size_t size)
        : data_(data), size_(size)
    {
    }

    FileReader::FileReader(const std::vector<uint8_t>& data)
        : data_(data.data())
        , size_(data.size())
    {
    }

    void FileReader::checkBounds(size_t required) const
    {
        if (required > size_)
        {
            throw std::out_of_range(
                "Position " + std::to_string(required) +
                " is out of range for data length " + std::to_string(size_));
        }
    }

    std::vector<uint8_t> FileReader::readBytes(size_t length)
    {
        checkBounds(position_ + length);
        std::vector<uint8_t> result(data_ + position_, data_ + position_ + length);
        position_ += length;
        return result;
    }

    void FileReader::skip(size_t length)
    {
        checkBounds(position_ + length);
        position_ += length;
    }

    uint8_t FileReader::readUint8()
    {
        checkBounds(position_ + 1);
        return data_[position_++];
    }

    int8_t FileReader::readInt8()
    {
        return static_cast<int8_t>(readUint8());
    }

    uint16_t FileReader::readUint16(bool littleEndian)
    {
        checkBounds(position_ + 2);
        uint16_t result;
        if (littleEndian) {
            result = static_cast<uint16_t>(data_[position_]) |
                (static_cast<uint16_t>(data_[position_ + 1]) << 8);
        }
        else {
            result = (static_cast<uint16_t>(data_[position_]) << 8) |
                static_cast<uint16_t>(data_[position_ + 1]);
        }
        position_ += 2;
        return result;
    }

    int16_t FileReader::readInt16()
    {
        return static_cast<int16_t>(readUint16(true));
    }

    uint32_t FileReader::readUint32()
    {
        checkBounds(position_ + 4);
        uint32_t result = static_cast<uint32_t>(data_[position_]) |
            (static_cast<uint32_t>(data_[position_ + 1]) << 8) |
            (static_cast<uint32_t>(data_[position_ + 2]) << 16) |
            (static_cast<uint32_t>(data_[position_ + 3]) << 24);
        position_ += 4;
        return result;
    }

    int32_t FileReader::readInt32()
    {
        return static_cast<int32_t>(readUint32());
    }

    float FileReader::readFloat32()
    {
        checkBounds(position_ + 4);
        float result;
        std::memcpy(&result, data_ + position_, 4);
        position_ += 4;
        return result;
    }

    double FileReader::readFloat64()
    {
        checkBounds(position_ + 8);
        double result;
        std::memcpy(&result, data_ + position_, 8);
        position_ += 8;
        return result;
    }

    void FileReader::setPosition(size_t position)
    {
        checkBounds(position);
        position_ = position;
    }

    bool FileReader::isEnd() const noexcept
    {
        return position_ == size_ - 1;
    }

}

