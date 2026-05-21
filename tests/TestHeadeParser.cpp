#include <gtest/gtest.h>
#include "../src/HeaderParser.h"
#include "../src/FileReader.h"
#include "../src/FontData.h"
#include <vector>
#include <string>
#include <stdexcept>

using shx::HeaderParser;
using shx::FileReader;
using shx::FontType;

static std::vector<uint8_t> makeHeaderBuffer(const std::string& header) {
    return std::vector<uint8_t>(header.begin(), header.end());
}

TEST(HeaderParser, ParseValidShapesHeader) {
    std::string hdr = "AutoCAD-86 shapes V1.0\r\n\x1a";
    auto buf = makeHeaderBuffer(hdr);
    FileReader reader(buf);
    HeaderParser parser;
    auto result = parser.parse(reader);

    EXPECT_EQ(result.fileHeader,  "AutoCAD-86");
    EXPECT_EQ(result.fontType,    FontType::SHAPES);
    EXPECT_EQ(result.fileVersion, "V1.0");
}

TEST(HeaderParser, ParseBigfontHeader) {
    std::string hdr = "AutoCAD-86 bigfont V1.0\r\n\x1a";
    auto buf = makeHeaderBuffer(hdr);
    FileReader reader(buf);
    HeaderParser parser;
    auto result = parser.parse(reader);

    EXPECT_EQ(result.fileHeader,  "AutoCAD-86");
    EXPECT_EQ(result.fontType,    FontType::BIGFONT);
    EXPECT_EQ(result.fileVersion, "V1.0");
}

TEST(HeaderParser, ParseUnifontHeader) {
    std::string hdr = "AutoCAD-86 unifont V1.0\r\n\x1a";
    auto buf = makeHeaderBuffer(hdr);
    FileReader reader(buf);
    HeaderParser parser;
    auto result = parser.parse(reader);

    EXPECT_EQ(result.fontType, FontType::UNIFONT);
}

TEST(HeaderParser, InvalidFontTypeThrows) {
    std::string hdr = "AutoCAD-86 INVALID V1.0\r\n\x1a";
    auto buf = makeHeaderBuffer(hdr);
    FileReader reader(buf);
    HeaderParser parser;
    EXPECT_THROW(parser.parse(reader), std::runtime_error);
}

TEST(HeaderParser, LongHeaderWithinLimits) {
    std::string hdr = "AutoCAD-86 unifont V1.0" +
                      std::string(900, ' ') + "\r\n\x1a";
    auto buf = makeHeaderBuffer(hdr);
    FileReader reader(buf);
    HeaderParser parser;
    auto result = parser.parse(reader);

    EXPECT_EQ(result.fileHeader,  "AutoCAD-86");
    EXPECT_EQ(result.fontType,    FontType::UNIFONT);
    EXPECT_EQ(result.fileVersion, "V1.0");
}
