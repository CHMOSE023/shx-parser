#pragma once
#include "Point.h"
#include <vector>
#include <optional>
#include <string>

namespace shx
{
    struct Box2d
    {
        double minX{ 0.0 }, minY{ 0.0 }, maxX{ 0.0 }, maxY{ 0.0 };
    };

    class ShxShape
    {
    public:
        std::optional<Point> lastPoint;
        std::vector<std::vector<Point>> polylines;

        ShxShape() = default;
        ShxShape(std::optional<Point> lastPoint,
            std::vector<std::vector<Point>> polylines);

        const Box2d& bbox() const;

        // Returns a new shape offset by p (isNewInstance=true) or modifies in-place (false).
        ShxShape offset(const Point& p, bool isNewInstance = true) const;
        ShxShape& offsetInPlace(const Point& p);

        // Translates so bbox bottom-left is at origin (0,0).
        ShxShape normalizeToOrigin(bool isNewInstance = false) const;

        std::string toSVG(const std::string& strokeWidth = "0.5%",
            const std::string& strokeColor = "black",
            bool isAutoFit = false) const;

    private:
        mutable std::optional<Box2d> bbox_;
    };

}
