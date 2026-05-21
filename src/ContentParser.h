#pragma once
#include "FontData.h"
#include "FileReader.h"
#include <memory>

namespace shx
{

    class ContentParser
    {
    public:
        virtual ~ContentParser() = default;
        virtual FontContentData parse(FileReader& reader) = 0;
    };

    class ContentParserFactory
    {
    public:
        static std::unique_ptr<ContentParser> createParser(FontType fontType);
    };

}  

