#include "HeaderParser.h"
#include <stdexcept>
#include <algorithm>
#include <cctype>

namespace shx
{

    static std::string trim(const std::string& s)
    {
        size_t start = s.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) return "";
        size_t end = s.find_last_not_of(" \t\r\n");
        return s.substr(start, end - start + 1);
    }

    static std::string toLower(std::string s)
    {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
            });
        return s;
    }

    FontHeaderData HeaderParser::parse(FileReader& reader) const
    {
        std::string headerData = parseHeader(reader);

        // Split on spaces: "AutoCAD-86 shapes V1.0"
        std::vector<std::string> parts;
        std::string token;
        for (char c : headerData) {
            if (c == ' ') {
                if (!token.empty()) { parts.push_back(token); token.clear(); }
            }
            else {
                token += c;
            }
        }
        if (!token.empty()) parts.push_back(token);

        if (parts.size() < 3) {
            throw std::runtime_error("Invalid SHX header: too few fields");
        }

        std::string fontTypeStr = toLower(parts[1]);
        FontType fontType;
        if (fontTypeStr == "shapes")  fontType = FontType::SHAPES;
        else if (fontTypeStr == "bigfont") fontType = FontType::BIGFONT;
        else if (fontTypeStr == "unifont") fontType = FontType::UNIFONT;
        else throw std::runtime_error("Invalid font type: " + fontTypeStr);

        return FontHeaderData{ fontType, parts[0], parts[2] };
    }

    std::string HeaderParser::parseHeader(FileReader& reader) const
    {
        std::string result;
        const size_t maxHeaderLength = 1024;
        size_t headerLength = 0;

        while (reader.currentPosition() < reader.length() - 2 &&
            headerLength < maxHeaderLength) {
            uint8_t byte1 = reader.readUint8();
            if (byte1 == 0x0D) {
                size_t savedPos = reader.currentPosition();
                uint8_t byte2 = reader.readUint8();
                uint8_t byte3 = reader.readUint8();
                if (byte2 == 0x0A && byte3 == 0x1A) {
                    break;
                }
                reader.setPosition(savedPos);
                result += static_cast<char>(byte1);
            }
            else {
                result += static_cast<char>(byte1);
            }
            ++headerLength;
        }

        return trim(result);
    }

}
