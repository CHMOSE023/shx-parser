#include "Font.h"
#include "FileReader.h"
#include "HeaderParser.h"
#include "ContentParser.h"

namespace shx
{
    ShxFont::ShxFont(const uint8_t* data, size_t size)
    {
        FileReader reader(data, size);
        HeaderParser hp;
        auto header = hp.parse(reader);
        auto contentParser = ContentParserFactory::createParser(header.fontType);
        auto content = contentParser->parse(reader);
        fontData_ = FontData{ header, std::move(content) };
        shapeParser_ = std::make_unique<ShapeParser>(fontData_);
    }

    ShxFont::ShxFont(const std::vector<uint8_t>& data)
        : ShxFont(data.data(), data.size())
    {
    }

    ShxFont::ShxFont(FontData fontData)
        : fontData_(std::move(fontData))
        , shapeParser_(std::make_unique<ShapeParser>(fontData_))
    {
    }

    bool ShxFont::hasChar(uint32_t code) const
    {
        return fontData_.content.data.count(code) > 0;
    }

    std::optional<ShxShape> ShxFont::getCharShape(uint32_t code, double size)
    {
        auto shape = shapeParser_->getCharShape(code, size);
        if (shape && fontData_.header.fontType == FontType::BIGFONT)
        {
            *shape = shape->normalizeToOrigin(true);
        }
        return shape;
    }

    void ShxFont::release() {
        shapeParser_->release();
    }

}

