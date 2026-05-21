#pragma once
#include "FontData.h"
#include "Shape.h"
#include "ShapeParser.h"
#include <optional>
#include <memory>
#include <vector>
#include <cstdint>

namespace shx
{

    class ShxFont 
    {
    public:
        explicit ShxFont(const uint8_t* data, size_t size);
        explicit ShxFont(const std::vector<uint8_t>& data);
        explicit ShxFont(FontData fontData);

        const FontData& fontData() const noexcept { return fontData_; }

        bool hasChar(uint32_t code) const;
        std::optional<ShxShape> getCharShape(uint32_t code, double size);
        void release();

    private:
        FontData fontData_;
        std::unique_ptr<ShapeParser> shapeParser_;
    };

} 
