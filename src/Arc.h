#pragma once
#include "Point.h"
#include <vector>
#include <cmath>

namespace shx
{

    /**
     * Circular arc defined either by bulge (start/end/bulge) or by octant
     * (center/radius/startOctant/octantCount/direction).
     *
     * Bulge convention: bulge = tan(theta/4), where theta is the included angle.
     * Positive bulge = counterclockwise. Negative = clockwise. 0 = straight line.
     * Bulge is clamped to [-1, 1] on creation.
     */
    class Arc
    {
    public:
        Point  start;
        Point  end;
        Point  center;
        double radius{ 0.0 };
        double startAngle{ 0.0 };
        double endAngle{ 0.0 };
        bool   isClockwise{ false };

        static Arc fromBulge(const Point& start, const Point& end, double bulge);
        static Arc fromOctant(const Point& center, double radius, int startOctant, int octantCount, bool isClockwise);

        std::vector<Point> tessellate(double circleSpan = M_PI / 18.0) const;

    private:
        Arc() = default;
    };

} // namespace shx
