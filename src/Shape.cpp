#include "Shape.h"
#include <limits>
#include <sstream>
#include <iomanip>
#include <cmath>

namespace shx
{

    ShxShape::ShxShape(std::optional<Point> lp, std::vector<std::vector<Point>> pl)
        : lastPoint(std::move(lp)), polylines(std::move(pl))
    {
    }

    const Box2d& ShxShape::bbox() const
    {
        if (bbox_) return *bbox_;

        double minX = std::numeric_limits<double>::infinity();
        double maxX = -std::numeric_limits<double>::infinity();
        double minY = std::numeric_limits<double>::infinity();
        double maxY = -std::numeric_limits<double>::infinity();

        for (const auto& polyline : polylines) {
            for (const auto& pt : polyline) {
                if (pt.x < minX) minX = pt.x;
                if (pt.x > maxX) maxX = pt.x;
                if (pt.y < minY) minY = pt.y;
                if (pt.y > maxY) maxY = pt.y;
            }
        }

        bbox_ = Box2d{ minX, minY, maxX, maxY };
        return *bbox_;
    }

    ShxShape ShxShape::offset(const Point& p, bool isNewInstance) const
    {
        if (!isNewInstance) {
            // Modifies the original - but this const overload can't do that.
            // For the in-place path, callers should use offsetInPlace().
            // Replicate the behavior of isNewInstance=false on a copy.
            ShxShape copy = *this;
            return copy.offsetInPlace(p);
        }

        std::optional<Point> newLast;
        if (lastPoint) {
            newLast = lastPoint->clone();
            newLast->add(p);
        }

        std::vector<std::vector<Point>> newPolylines;
        newPolylines.reserve(polylines.size());
        for (const auto& line : polylines) {
            std::vector<Point> newLine;
            newLine.reserve(line.size());
            for (const auto& pt : line) {
                newLine.push_back(pt.clone().add(p));
            }
            newPolylines.push_back(std::move(newLine));
        }
        return ShxShape(std::move(newLast), std::move(newPolylines));
    }

    ShxShape& ShxShape::offsetInPlace(const Point& p)
    {
        if (lastPoint) lastPoint->add(p);
        for (auto& line : polylines)
            for (auto& pt : line)
                pt.add(p);
        if (bbox_) {
            bbox_->minX += p.x; bbox_->maxX += p.x;
            bbox_->minY += p.y; bbox_->maxY += p.y;
        }
        return *this;
    }

    ShxShape ShxShape::normalizeToOrigin(bool /*isNewInstance*/) const
    {
        const Box2d& b = bbox();
        Point shift(-b.minX, -b.minY);
        return offset(shift, true);
    }

    static std::string fmtDouble(double v)
    {
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(2) << v;
        return ss.str();
    }

    std::string ShxShape::toSVG(const std::string& strokeWidth,
        const std::string& strokeColor,
        bool isAutoFit) const
    {
        std::ostringstream paths;
        std::string viewBox;

        auto makePath = [&](const std::vector<std::vector<Point>>& lines,
            double offX, double offY) {
                for (const auto& polyline : lines) {
                    if (polyline.empty()) continue;
                    paths << "<path d=\"";
                    for (size_t i = 0; i < polyline.size(); ++i) {
                        double px = polyline[i].x + offX;
                        double py = -polyline[i].y + offY; // flip Y for SVG
                        paths << (i == 0 ? "M " : "L ")
                            << fmtDouble(px) << " " << fmtDouble(py) << " ";
                    }
                    paths << "\" stroke=\"" << strokeColor
                          << "\" stroke-width=\"" << strokeWidth
                          << "\" fill=\"none\"/>";
                }
            };

        if (isAutoFit) {
            const Box2d& b = bbox();
            double rawW = b.maxX - b.minX;
            double rawH = b.maxY - b.minY;
            double w = rawW == 0.0 ? rawH : rawW;
            double h = rawH == 0.0 ? rawW : rawH;
            const double pad = 0.2;
            double vbMinX = b.minX - w * pad;
            double vbMaxX = b.maxX + w * pad;
            double vbMinY = b.minY - h * pad;
            double vbMaxY = b.maxY + h * pad;
            viewBox = fmtDouble(vbMinX) + " " + fmtDouble(-vbMaxY) + " " +
                fmtDouble(vbMaxX - vbMinX) + " " + fmtDouble(vbMaxY - vbMinY);
            makePath(polylines, 0.0, 0.0);
        }
        else {
            viewBox = "0 0 20 20";
            makePath(polylines, 5.0, 15.0);
        }

        return "<svg width=\"100%\" height=\"100%\" viewBox=\"" + viewBox +
            "\" preserveAspectRatio=\"xMidYMid meet\">" + paths.str() + "</svg>";
    }

}
