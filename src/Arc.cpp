#include "Arc.h"
#include <algorithm>
#include <stdexcept>
#include <cmath>

namespace shx
{

    static constexpr double OCTANT_ANGLE = M_PI / 4.0;

    Arc Arc::fromBulge(const Point& start, const Point& end, double bulge)
    {
        double clamped = std::max(-1.0, std::min(1.0, bulge));

        Arc arc;
        arc.start = start.clone();
        arc.end = end.clone();
        arc.isClockwise = clamped < 0.0;

        Point distance = end.clone().subtract(start);
        double D = distance.length();
        double H = (std::abs(clamped) * D) / 2.0;

        if (H == 0.0) {
            arc.radius = 0.0;
            arc.center = start.clone();
            arc.startAngle = std::atan2(distance.y, distance.x);
            arc.endAngle = arc.startAngle;
            return arc;
        }

        double theta = 4.0 * std::atan(std::abs(clamped));
        arc.radius = D / (2.0 * std::sin(theta / 2.0));

        Point midpoint = start.clone().add(distance.clone().divide(2.0));
        Point normal(-distance.y, distance.x);
        normal.normalize();
        normal.multiply(std::abs(arc.radius * std::cos(theta / 2.0)));

        arc.center = midpoint.clone();
        if (arc.isClockwise) {
            arc.center.subtract(normal);
        }
        else {
            arc.center.add(normal);
        }

        arc.startAngle = std::atan2(arc.start.y - arc.center.y, arc.start.x - arc.center.x);
        arc.endAngle = std::atan2(arc.end.y - arc.center.y, arc.end.x - arc.center.x);

        if (arc.isClockwise) {
            if (arc.endAngle >= arc.startAngle) arc.endAngle -= 2.0 * M_PI;
        }
        else {
            if (arc.endAngle <= arc.startAngle) arc.endAngle += 2.0 * M_PI;
        }

        return arc;
    }

    Arc Arc::fromOctant(const Point& center, double radius,
        int startOctant, int octantCount, bool isClockwise)
    {
        Arc arc;
        arc.center = center.clone();
        arc.radius = radius;
        arc.isClockwise = isClockwise;

        arc.startAngle = startOctant * OCTANT_ANGLE;
        double span = (octantCount == 0 ? 8 : octantCount) * OCTANT_ANGLE;
        arc.endAngle = arc.startAngle + (isClockwise ? -span : span);

        arc.start = center.clone().add(
            Point(radius * std::cos(arc.startAngle), radius * std::sin(arc.startAngle)));
        arc.end = center.clone().add(
            Point(radius * std::cos(arc.endAngle), radius * std::sin(arc.endAngle)));

        return arc;
    }

    std::vector<Point> Arc::tessellate(double circleSpan) const
    {
        if (radius == 0.0) {
            return { start.clone(), end.clone() };
        }

        std::vector<Point> points;
        points.push_back(start.clone());

        double includedAngle = std::abs(endAngle - startAngle);
        int numSegments = std::max(1, static_cast<int>(std::floor(includedAngle / circleSpan)));

        for (int i = 1; i < numSegments; ++i) {
            double t = static_cast<double>(i) / numSegments;
            double angle = isClockwise
                ? startAngle - t * includedAngle
                : startAngle + t * includedAngle;
            points.push_back(center.clone().add(
                Point(radius * std::cos(angle), radius * std::sin(angle))));
        }

        points.push_back(center.clone().add(
            Point(radius * std::cos(endAngle), radius * std::sin(endAngle))));

        return points;
    }

}

