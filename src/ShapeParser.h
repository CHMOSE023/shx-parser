#pragma once
#include "FontData.h"
#include "Shape.h"
#include <unordered_map>
#include <optional>
#include <vector>

namespace shx 
{

    struct ScalingOptions
    {
        std::optional<double> factor;
        std::optional<double> height;
        std::optional<double> width;
    };

    class ShapeParser 
    {
    public:
        explicit ShapeParser(const FontData& fontData);

        void release();
        std::optional<ShxShape> getCharShape(uint32_t code, double size);

    private:
        const FontData& fontData_;
        std::unordered_map<uint32_t, ShxShape> shapeCache_;

        std::optional<ShxShape> parseAndScale(uint32_t code, const ScalingOptions& options);
        ShxShape scaleShapeByFactor(const ShxShape& shape, double factor) const;
        ShxShape scaleShapeByHeightAndWidth(const ShxShape& shape, double height, double width) const;
        ShxShape parseShape(const std::vector<uint8_t>& data);

        struct ParseState {
            Point currentPoint;
            std::vector<std::vector<Point>> polylines;
            std::vector<Point> currentPolyline;
            std::vector<Point> sp; // position stack (max depth 4)
            bool isPenDown{ false };
            double scale{ 1.0 };
        };

        size_t handleSpecialCommand(uint8_t command, const std::vector<uint8_t>& data, size_t index, ParseState& state);
        void   handleVectorCommand(uint8_t command, ParseState& state);
        size_t handleSubshapeCommand(const std::vector<uint8_t>& data, size_t index, ParseState& state);
        size_t handleXYDisplacement(const std::vector<uint8_t>& data, size_t index, ParseState& state);
        size_t handleMultipleXYDisplacements(const std::vector<uint8_t>& data, size_t index, ParseState& state);
        size_t handleOctantArc(const std::vector<uint8_t>& data, size_t index, ParseState& state);
        size_t handleFractionalArc(const std::vector<uint8_t>& data, size_t index, ParseState& state);
        size_t handleBulgeArc(const std::vector<uint8_t>& data, size_t index, ParseState& state);
        size_t handleMultipleBulgeArcs(const std::vector<uint8_t>& data, size_t index, ParseState& state);
        size_t skipCode(const std::vector<uint8_t>& data, size_t index);

        std::optional<ShxShape> getScaledSubshapeAtInsertPoint(uint32_t code, double width,
            double height, const Point& insertPoint);
        Point handleArcSegment(const Point& currentPoint, Point vec, int8_t bulge, double scale,
            bool isPenDown, std::vector<Point>& currentPolyline);
        Point getVectorForDirection(int dir) const noexcept;
    };

}
