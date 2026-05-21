#include "../src/ShxParser.h"

#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

// --- File helpers -------------------------------------------------------------

static std::vector<uint8_t> readFile(const std::string& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file) throw std::runtime_error("Cannot open file: " + path);
    return { std::istreambuf_iterator<char>(file),
            std::istreambuf_iterator<char>() };
}

static void writeFile(const std::string& path, const std::string& content)
{
    std::ofstream out(path);
    if (!out) throw std::runtime_error("Cannot write file: " + path);
    out << content;
}

// --- Console output -----------------------------------------------------------

static std::string fontTypeName(shx::FontType t)
{
    switch (t) {
    case shx::FontType::SHAPES:  return "shapes";
    case shx::FontType::BIGFONT: return "bigfont";
    case shx::FontType::UNIFONT: return "unifont";
    }
    return "unknown";
}

static void printFontInfo(const shx::FontData& fd)
{
    const auto& h = fd.header;
    const auto& c = fd.content;

    std::cout << "\nFont Information:\n"
        << "----------------\n"
        << "Font Type   : " << fontTypeName(h.fontType) << "\n"
        << "Header      : " << h.fileHeader << "\n"
        << "Version     : " << h.fileVersion << "\n"
        << "Info        : " << c.info << "\n"
        << "Orientation : " << c.orientation << "\n"
        << "Base Up     : " << c.baseUp
        << " / Base Down: " << c.baseDown << "\n"
        << "Height      : " << c.height
        << " / Width   : " << c.width << "\n"
        << "Shapes count: " << c.data.size() << "\n";

    // Print up to 30 available codes (sorted)
    std::vector<uint32_t> codes;
    codes.reserve(c.data.size());
    for (const auto& kv : c.data) codes.push_back(kv.first);
    std::sort(codes.begin(), codes.end());

    std::cout << "Available codes (first 30): ";
    size_t limit = std::min<size_t>(30, codes.size());
    for (size_t i = 0; i < limit; ++i) {
        std::cout << codes[i];
        if (i + 1 < limit) std::cout << ", ";
    }
    if (codes.size() > 30) std::cout << " ...";
    std::cout << "\n----------------\n";
}

// --- Shape info to console ----------------------------------------------------

static void printShapeInfo(char ch, const shx::ShxShape& shape)
{
    std::cout << "  Char '" << ch << "' (code " << static_cast<int>(ch) << ")"
        << "  polylines=" << shape.polylines.size();
    if (shape.lastPoint)
        std::cout << "  lastPoint=(" << std::fixed << std::setprecision(2)
        << shape.lastPoint->x << ", " << shape.lastPoint->y << ")";
    std::cout << "\n";

    for (size_t li = 0; li < shape.polylines.size(); ++li) {
        const auto& line = shape.polylines[li];
        if (line.empty()) continue;
        const auto& first = line.front();
        const auto& last = line.back();
        std::cout << "    polyline[" << li << "]  pts=" << line.size()
            << "  start=(" << std::fixed << std::setprecision(2)
            << first.x << "," << first.y << ")"
            << "  end=(" << last.x << "," << last.y << ")\n";
    }
}

// --- Shape to SVG path string -------------------------------------------------

static std::string shapeToSvgPath(const shx::ShxShape& shape, double offsetX, double offsetY)
{
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2);
    for (const auto& polyline : shape.polylines) {
        if (polyline.empty()) continue;
        for (size_t i = 0; i < polyline.size(); ++i) {
            double px = polyline[i].x + offsetX;
            double py = -polyline[i].y + offsetY; // flip Y for SVG coordinate system
            ss << (i == 0 ? "M " : "L ") << px << " " << py << " ";
        }
    }
    return ss.str();
}

// --- Generate SVG for a text string ------------------------------------------

static std::string generateSvg(const std::string& text, shx::ShxFont& font, double fontSize)
{
    const double padding = fontSize;
    double currentX = padding;
    double maxHeight = 0.0;

    struct PathEntry { std::string d; };
    std::vector<PathEntry> paths;

    for (char ch : text) {
        uint32_t code = static_cast<uint32_t>(static_cast<unsigned char>(ch));
        auto shapeOpt = font.getCharShape(code, fontSize);
        if (!shapeOpt) {
            std::cerr << "  Warning: no shape for '" << ch
                << "' (code " << code << ")\n";
            currentX += fontSize;
            continue;
        }

        std::string d = shapeToSvgPath(*shapeOpt, currentX, padding);
        if (!d.empty())
            paths.push_back({ std::move(d) });

        // Advance cursor by the character's lastPoint.x (advance width)
        if (shapeOpt->lastPoint)
            currentX += shapeOpt->lastPoint->x + fontSize * 0.5;
        else
            currentX += fontSize;

        maxHeight = std::max(maxHeight, fontSize);
    }

    double svgWidth = currentX + padding;
    double svgHeight = maxHeight + padding * 2.0;

    std::ostringstream svg;
    svg << std::fixed << std::setprecision(2);
    svg << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n"
        << "<svg width=\"" << svgWidth
        << "\" height=\"" << svgHeight
        << "\" viewBox=\"0 0 " << svgWidth << " " << svgHeight << "\""
        << " xmlns=\"http://www.w3.org/2000/svg\">\n";

    for (const auto& p : paths)
        svg << "  <path d=\"" << p.d
        << "\" fill=\"none\" stroke=\"black\" stroke-width=\"1\"/>\n";

    svg << "</svg>\n";
    return svg.str();
}

// --- Process one font file ----------------------------------------------------

static void processFont(const std::string& filePath, const std::string& svgOutputPath, const std::string& renderText, double fontSize)
{
    std::cout << "\n========================================\n";
    std::cout << "Reading font file: " << filePath << "\n";

    auto buffer = readFile(filePath);
    std::cout << "File size: " << buffer.size() << " bytes\n";

    shx::ShxFont font(buffer);
    printFontInfo(font.fontData());

    std::cout << "\nShape details for \"" << renderText
        << "\" at size " << fontSize << ":\n";
    for (char ch : renderText) {
        uint32_t code = static_cast<uint32_t>(static_cast<unsigned char>(ch));
        auto shapeOpt = font.getCharShape(code, fontSize);
        if (shapeOpt)
            printShapeInfo(ch, *shapeOpt);
        else
            std::cout << "  Char '" << ch << "' (code " << code
            << "): no shape found\n";
    }

    std::cout << "\nGenerating SVG for: \"" << renderText << "\"\n";
    std::string svg = generateSvg(renderText, font, fontSize);
    writeFile(svgOutputPath, svg);
    std::cout << "SVG saved to: " << svgOutputPath << "\n";

    font.release();
}

// --- main --------------------------------------------------------------------

int main() 
{
    const std::string dataDir = SHX_DATA_DIR; // injected by CMake

    // Resolve the examples/ source directory for SVG output
    std::string outDir = ".";
    try {
        outDir = fs::canonical(
            fs::path(dataDir) / ".." / "cpp" / "examples").string();
    }
    catch (...) {}

    const std::string renderText = "ABCDEF123abc";
    const double      fontSize   = 12.0;

    try 
    {
     /*   processFont(dataDir + "/ISO.shx",
            outDir + "/iso_output.svg",
            renderText, fontSize);

        processFont(dataDir + "/SIMPLEX8.shx",
            outDir + "/simplex8_output.svg",
            renderText, fontSize);*/

        processFont(dataDir + "/tssdchn.shx",
            outDir + "/tssdchn_output.svg",
            renderText, fontSize);

      /*  processFont(dataDir + "/tssdeng.shx",
            outDir + "/tssdeng_output.svg",
            renderText, fontSize);*/

    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    std::cout << "\nDone.\n";
    return 0;
}
