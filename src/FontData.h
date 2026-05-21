#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>

namespace shx
{

    enum class FontType 
    {
        SHAPES,
        BIGFONT,
        UNIFONT
    };

    struct FontHeaderData 
    {
        FontType fontType{ FontType::SHAPES };
        std::string fileHeader;
        std::string fileVersion;
    };

    struct FontContentData 
    {
        std::unordered_map<uint32_t, std::vector<uint8_t>> data;
        std::string info;
        std::string orientation{ "horizontal" };
        int baseUp{ 8 };
        int baseDown{ 2 };
        double height{ 10.0 };
        double width{ 10.0 };
        bool isExtended{ false };
    };

    struct FontData 
    {
        FontHeaderData header;
        FontContentData content;
    };

} 

