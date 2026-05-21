#include "ContentParser.h"
#include <stdexcept>
#include <iostream>
#include <string>

namespace shx 
{
    namespace
    {

        static const double DEFAULT_FONT_SIZE = 10.0;
        // Bytes that terminate the info string
        static const uint8_t TERM_CR = 0x0D;
        static const uint8_t TERM_LF = 0x0A;
        static const uint8_t TERM_NUL = 0x00;

        // ─── Shape font ──────────────────────────────────────────────────────────────

        class ShapeContentParser : public ContentParser 
        {
        public:
            FontContentData parse(FileReader& reader) override {
                FontContentData fd;
                fd.height = DEFAULT_FONT_SIZE;
                fd.width = DEFAULT_FONT_SIZE;

                try {
                    reader.readBytes(4); // skip start+end codes

                    int16_t count = reader.readInt16();
                    if (count <= 0) throw std::runtime_error("Invalid shape count");

                    struct Item { uint16_t code; uint16_t length; };
                    std::vector<Item> items;
                    items.reserve(static_cast<size_t>(count));

                    for (int i = 0; i < count; ++i) {
                        uint16_t code = reader.readUint16();
                        uint16_t length = reader.readUint16();
                        if (length > 0) items.push_back({ code, length });
                    }

                    for (const auto& item : items) {
                        try {
                            auto bytes = reader.readBytes(item.length);
                            if (bytes.size() == item.length)
                                fd.data[item.code] = std::move(bytes);
                        }
                        catch (...) {
                            std::cerr << "Warning: failed to read shape data for code "
                                << item.code << "\n";
                        }
                    }

                    // Parse info block (code 0)
                    auto it = fd.data.find(0);
                    if (it != fd.data.end()) {
                        const auto& infoData = it->second;
                        try {
                            size_t idx = std::string::npos;
                            for (size_t i = 0; i < infoData.size(); ++i) {
                                if (infoData[i] == TERM_CR || infoData[i] == TERM_LF ||
                                    infoData[i] == TERM_NUL) {
                                    idx = i; break;
                                }
                            }
                            if (idx != std::string::npos) {
                                fd.info = std::string(infoData.begin(),
                                    infoData.begin() + static_cast<ptrdiff_t>(idx));
                                if (idx + 3 < infoData.size()) {
                                    fd.baseUp = infoData[idx + 1];
                                    fd.baseDown = infoData[idx + 2];
                                    fd.height = fd.baseDown + fd.baseUp;
                                    fd.width = fd.height;
                                    fd.orientation = (infoData[idx + 3] == 0) ? "horizontal" : "vertical";
                                }
                            }
                        }
                        catch (...) {
                            std::cerr << "Warning: failed to parse font info block\n";
                        }
                    }
                }
                catch (const std::exception& e) {
                    std::cerr << "Error parsing shape font: " << e.what() << "\n";
                    fd.data.clear();
                    fd.info = "Failed to parse font file";
                }
                return fd;
            }
        };

        // ─── Big font ─────────────────────────────────────────────────────────────────

        struct Utf8Result { std::string text; size_t pos; };

        // Mirrors the TypeScript utf8ArrayToStr used for bigfont info blocks.
        static Utf8Result utf8ArrayToStr(const std::vector<uint8_t>& arr) 
        {
            std::string out;
            size_t i = 0;
            while (i < arr.size()) {
                uint8_t c = arr[i];
                int hi = c >> 4;
                if (hi <= 7) {
                    out += static_cast<char>(c);
                }
                else if (hi == 12 || hi == 13) {
                    if (i + 1 >= arr.size()) break;
                    uint8_t c2 = arr[++i];
                    uint32_t ch = static_cast<uint32_t>((c & 0x1F) << 6) |
                        static_cast<uint32_t>(c2 & 0x3F);
                    if (ch < 0x80) {
                        out += static_cast<char>(ch);
                    }
                    else {
                        out += static_cast<char>(0xC0 | (ch >> 6));
                        out += static_cast<char>(0x80 | (ch & 0x3F));
                    }
                }
                else if (hi == 14) {
                    if (i + 2 >= arr.size()) break;
                    uint8_t c2 = arr[++i];
                    uint8_t c3 = arr[++i];
                    uint32_t ch = static_cast<uint32_t>((c & 0x0F) << 12) |
                        static_cast<uint32_t>((c2 & 0x3F) << 6) |
                        static_cast<uint32_t>(c3 & 0x3F);
                    if (ch < 0x800) {
                        out += static_cast<char>(0xC0 | (ch >> 6));
                        out += static_cast<char>(0x80 | (ch & 0x3F));
                    }
                    else {
                        out += static_cast<char>(0xE0 | (ch >> 12));
                        out += static_cast<char>(0x80 | ((ch >> 6) & 0x3F));
                        out += static_cast<char>(0x80 | (ch & 0x3F));
                    }
                }
                // Stop on null character
                if (!out.empty() && static_cast<unsigned char>(out.back()) == 0) break;
                ++i;
            }
            return { out, i };
        }

        class BigfontContentParser : public ContentParser
        {
        public:
            FontContentData parse(FileReader& reader) override 
            {
                FontContentData fd;
                fd.height = DEFAULT_FONT_SIZE;
                fd.width = DEFAULT_FONT_SIZE;

                try {
                    reader.readInt16(); // item length (unused)
                    int16_t count = reader.readInt16();
                    int16_t changeNumber = reader.readInt16();

                    if (count <= 0) throw std::runtime_error("Invalid character count");

                    reader.skip(static_cast<size_t>(changeNumber) * 4);

                    struct Item { uint16_t code; uint16_t length; uint32_t offset; };
                    std::vector<Item> items;
                    items.reserve(static_cast<size_t>(count));

                    for (int i = 0; i < count; ++i) {
                        uint16_t code = reader.readUint16();
                        uint16_t length = reader.readUint16();
                        uint32_t offset = reader.readUint32();
                        if (code != 0 || length != 0 || offset != 0)
                            items.push_back({ code, length, offset });
                    }

                    for (const auto& item : items) {
                        try {
                            reader.setPosition(item.offset);
                            auto bytes = reader.readBytes(item.length);
                            if (bytes.size() == item.length)
                                fd.data[item.code] = std::move(bytes);
                        }
                        catch (...) {
                            std::cerr << "Warning: failed to read bigfont data for code "
                                << item.code << "\n";
                        }
                    }

                    // Parse info block (code 0)
                    auto it = fd.data.find(0);
                    if (it != fd.data.end()) {
                        const auto& infoData = it->second;
                        try {
                            Utf8Result info = utf8ArrayToStr(infoData);
                            size_t idx = info.pos;
                            if (idx < infoData.size()) {
                                fd.info = info.text;
                                ++idx; // move past null terminator
                                if (infoData.size() - idx > 4) {
                                    // Extended big font: height, skip, orientation, width
                                    fd.height = infoData[idx++];
                                    ++idx; // skip
                                    fd.orientation = (infoData[idx++] == 0) ? "horizontal" : "vertical";
                                    fd.width = infoData[idx];
                                    fd.baseUp = static_cast<int>(fd.height);
                                    fd.baseDown = 0;
                                    fd.isExtended = true;
                                }
                                else if (idx + 3 <= infoData.size()) {
                                    fd.baseUp = infoData[idx++];
                                    fd.baseDown = infoData[idx++];
                                    fd.height = fd.baseDown + fd.baseUp;
                                    fd.width = fd.height;
                                    fd.orientation = (infoData[idx] == 0) ? "horizontal" : "vertical";
                                }
                            }
                        }
                        catch (...) {
                            std::cerr << "Warning: failed to parse bigfont info block\n";
                        }
                    }
                }
                catch (const std::exception& e) {
                    std::cerr << "Error parsing big font: " << e.what() << "\n";
                    fd.data.clear();
                    fd.info = "Failed to parse font file";
                }
                return fd;
            }
        };

        // ─── Uni font ─────────────────────────────────────────────────────────────────

        class UnifontContentParser : public ContentParser 
        {
        public:
            FontContentData parse(FileReader& reader) override {
                FontContentData fd;
                fd.height = DEFAULT_FONT_SIZE;
                fd.width = DEFAULT_FONT_SIZE;

                try {
                    int32_t count = reader.readInt32();
                    if (count <= 0) throw std::runtime_error("Invalid character count");

                    int16_t infoLength = reader.readInt16();
                    auto infoData = reader.readBytes(static_cast<size_t>(infoLength));

                    // Parse info data
                    try {
                        size_t idx = std::string::npos;
                        for (size_t i = 0; i < infoData.size(); ++i) {
                            if (infoData[i] == TERM_NUL) { idx = i; break; }
                        }
                        if (idx != std::string::npos) {
                            fd.info = std::string(infoData.begin(),
                                infoData.begin() + static_cast<ptrdiff_t>(idx));
                            if (idx + 3 < infoData.size()) {
                                fd.baseUp = infoData[idx + 1];
                                fd.baseDown = infoData[idx + 2];
                                fd.height = fd.baseUp + fd.baseDown;
                                fd.width = fd.height;
                                fd.orientation = (infoData[idx + 3] == 0) ? "horizontal" : "vertical";
                            }
                        }
                    }
                    catch (...) {
                        std::cerr << "Warning: failed to parse unifont info block\n";
                    }

                    for (int i = 0; i < count - 1; ++i) {
                        try {
                            uint16_t code = reader.readUint16();
                            uint16_t length = reader.readUint16();
                            if (length > 0) {
                                auto bytes = reader.readBytes(length);
                                if (bytes.size() == length) {
                                    // Skip null-terminated label at the start of each entry
                                    size_t nulIdx = std::string::npos;
                                    for (size_t j = 0; j < bytes.size(); ++j) {
                                        if (bytes[j] == 0x00) { nulIdx = j; break; }
                                    }
                                    size_t startOfBytecode = (nulIdx != std::string::npos)
                                        ? nulIdx + 1 : 0;
                                    if (startOfBytecode < bytes.size()) {
                                        fd.data[code] = std::vector<uint8_t>(
                                            bytes.begin() + static_cast<ptrdiff_t>(startOfBytecode),
                                            bytes.end());
                                    }
                                }
                            }
                        }
                        catch (...) {
                            std::cerr << "Warning: failed to read unifont character data\n";
                            break;
                        }
                    }
                }
                catch (const std::exception& e) {
                    std::cerr << "Error parsing unifont: " << e.what() << "\n";
                    fd.data.clear();
                    fd.info = "Failed to parse font file";
                }
                return fd;
            }
        };

    } // anonymous namespace

    // ─── Factory ──────────────────────────────────────────────────────────────────

    std::unique_ptr<ContentParser> ContentParserFactory::createParser(FontType fontType) 
    {
        switch (fontType) {
        case FontType::SHAPES:  return std::make_unique<ShapeContentParser>();
        case FontType::BIGFONT: return std::make_unique<BigfontContentParser>();
        case FontType::UNIFONT: return std::make_unique<UnifontContentParser>();
        default:
            throw std::runtime_error("Unsupported font type");
        }
    }

} 
