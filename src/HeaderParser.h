#pragma once
#include "FontData.h"
#include "FileReader.h"

namespace shx
{
    class HeaderParser
    {
    public:
        FontHeaderData parse(FileReader& reader) const;

    private:
        std::string parseHeader(FileReader& reader) const;
    };

} 
