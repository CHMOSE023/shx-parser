#include "ShapeParser.h"
#include "Arc.h"
#include "FileReader.h"
#include <stdexcept>
#include <cmath>

namespace shx {

    static constexpr double CIRCLE_SPAN = M_PI / 18.0;

    ShapeParser::ShapeParser(const FontData& fontData)
        : fontData_(fontData)
    {
    }

    void ShapeParser::release() 
    {
        shapeCache_.clear();
    }

    std::optional<ShxShape> ShapeParser::getCharShape(uint32_t code, double size) 
    {
        double scale = size / fontData_.content.height;
        return parseAndScale(code, { scale, std::nullopt, std::nullopt });
    }

    std::optional<ShxShape> ShapeParser::parseAndScale(uint32_t code, const ScalingOptions& options) 
    {
        if (code == 0) return std::nullopt;

        ShxShape* baseShape = nullptr;

        auto cacheIt = shapeCache_.find(code);
        if (cacheIt != shapeCache_.end()) {
            baseShape = &cacheIt->second;
        }
        else {
            auto dataIt = fontData_.content.data.find(code);
            if (dataIt == fontData_.content.data.end()) return std::nullopt;
            shapeCache_.emplace(code, parseShape(dataIt->second));
            baseShape = &shapeCache_.at(code);
        }

        if (options.factor.has_value()) {
            return scaleShapeByFactor(*baseShape, *options.factor);
        }
        else if (options.height.has_value()) {
            double tw = options.width.value_or(*options.height);
            return scaleShapeByHeightAndWidth(*baseShape, *options.height, tw);
        }
        return *baseShape;
    }

    ShxShape ShapeParser::scaleShapeByFactor(const ShxShape& shape, double factor) const
    {
        std::optional<Point> newLast;
        if (shape.lastPoint) {
            newLast = shape.lastPoint->clone().multiply(factor);
        }
        std::vector<std::vector<Point>> newPolylines;
        newPolylines.reserve(shape.polylines.size());
        for (const auto& line : shape.polylines) {
            std::vector<Point> newLine;
            newLine.reserve(line.size());
            for (const auto& pt : line)
                newLine.push_back(pt.clone().multiply(factor));
            newPolylines.push_back(std::move(newLine));
        }
        return ShxShape(std::move(newLast), std::move(newPolylines));
    }

    ShxShape ShapeParser::scaleShapeByHeightAndWidth(const ShxShape& shape,
        double height, double width) const 
    {
        const Box2d& b = shape.bbox();
        double shapeH = b.maxY - b.minY;
        double shapeW = b.maxX - b.minX;
        double hs = (shapeH > 0.0) ? height / shapeH : 1.0;
        double ws = (shapeW > 0.0) ? width / shapeW : 1.0;

        std::optional<Point> newLast;
        if (shape.lastPoint) {
            newLast = shape.lastPoint->clone();
            newLast->x *= ws;
            newLast->y *= hs;
        }

        std::vector<std::vector<Point>> newPolylines;
        newPolylines.reserve(shape.polylines.size());
        for (const auto& line : shape.polylines) {
            std::vector<Point> newLine;
            newLine.reserve(line.size());
            for (const auto& pt : line) {
                Point p = pt.clone();
                p.x *= ws;
                p.y *= hs;
                newLine.push_back(p);
            }
            newPolylines.push_back(std::move(newLine));
        }
        return ShxShape(std::move(newLast), std::move(newPolylines));
    }

    ShxShape ShapeParser::parseShape(const std::vector<uint8_t>& data)
    {
        ParseState state;

        for (size_t i = 0; i < data.size(); ++i) {
            uint8_t cb = data[i];
            if (cb <= 0x0F) {
                i = handleSpecialCommand(cb, data, i, state);
            }
            else {
                handleVectorCommand(cb, state);
            }
        }

        return ShxShape(state.currentPoint, state.polylines);
    }

    size_t ShapeParser::handleSpecialCommand(uint8_t command,
        const std::vector<uint8_t>& data,
        size_t index, ParseState& state) 
    {
        size_t i = index;
        switch (command) {
        case 0: // End of shape
            state.currentPolyline.clear();
            state.isPenDown = false;
            break;
        case 1: // Pen down
            state.isPenDown = true;
            state.currentPolyline.push_back(state.currentPoint.clone());
            break;
        case 2: // Pen up
            state.isPenDown = false;
            if (state.currentPolyline.size() > 1)
                state.polylines.push_back(state.currentPolyline);
            state.currentPolyline.clear();
            break;
        case 3: // Divide vector lengths
            ++i;
            state.scale /= data[i];
            break;
        case 4: // Multiply vector lengths
            ++i;
            state.scale *= data[i];
            break;
        case 5: // Push location
            if (state.sp.size() == 4)
                throw std::runtime_error("The position stack is only four locations deep");
            state.sp.push_back(state.currentPoint.clone());
            break;
        case 6: // Pop location
            if (!state.sp.empty()) {
                state.currentPoint = state.sp.back();
                state.sp.pop_back();
            }
            break;
        case 7: // Draw subshape
            i = handleSubshapeCommand(data, i, state);
            break;
        case 8: // X-Y displacement
            i = handleXYDisplacement(data, i, state);
            break;
        case 9: // Multiple X-Y displacements
            i = handleMultipleXYDisplacements(data, i, state);
            break;
        case 10: // Octant arc
            i = handleOctantArc(data, i, state);
            break;
        case 11: // Fractional arc
            i = handleFractionalArc(data, i, state);
            break;
        case 12: // Bulge arc
            i = handleBulgeArc(data, i, state);
            break;
        case 13: // Multiple bulge arcs
            i = handleMultipleBulgeArcs(data, i, state);
            break;
        case 14: // Vertical text - skip the following code
            i = skipCode(data, ++i);
            break;
        default:
            break;
        }
        return i;
    }

    void ShapeParser::handleVectorCommand(uint8_t command, ParseState& state) 
    {
        int len = (command & 0xF0) >> 4;
        int dir = command & 0x0F;
        Point vec = getVectorForDirection(dir);
        state.currentPoint.add(vec.multiply(static_cast<double>(len) * state.scale));
        if (state.isPenDown)
            state.currentPolyline.push_back(state.currentPoint.clone());
    }

    size_t ShapeParser::handleSubshapeCommand(const std::vector<uint8_t>& data,
        size_t index, ParseState& state)
    {
        size_t i = index;
        uint32_t subCode = 0;
        double height = state.scale * fontData_.content.height;
        double width = height;
        Point origin = state.currentPoint.clone();

        if (state.currentPolyline.size() > 1) {
            state.polylines.push_back(state.currentPolyline);
            state.currentPolyline.clear();
        }

        switch (fontData_.header.fontType) {
        case FontType::SHAPES:
            ++i;
            subCode = data[i];
            break;

        case FontType::BIGFONT:
            ++i;
            subCode = data[i];
            if (subCode == 0) {
                ++i;
                // Read 2-byte primitive# (high byte first in data order)
                uint32_t hi = data[i++];
                uint32_t lo = data[i++];
                subCode = (hi << 8) | lo;
                origin.x = FileReader::byteToSByte(data[i++]) * state.scale;
                origin.y = FileReader::byteToSByte(data[i++]) * state.scale;
                if (fontData_.content.isExtended) {
                    width = data[i++] * state.scale;
                    height = data[i] * state.scale;
                }
                else {
                    height = data[i] * state.scale;
                }
            }
            break;

        case FontType::UNIFONT:
            ++i;
            {
                uint32_t hi = data[i++];
                uint32_t lo = data[i++];
                subCode = (hi << 8) | lo;
            }
            break;
        }

        if (subCode != 0) {
            auto sub = getScaledSubshapeAtInsertPoint(subCode, width, height, origin);
            if (sub) {
                for (auto& line : sub->polylines)
                    state.polylines.push_back(std::move(line));
            }
        }

        state.currentPolyline.clear();
        return i;
    }

    size_t ShapeParser::handleXYDisplacement(const std::vector<uint8_t>& data,
        size_t index, ParseState& state)
    {
        size_t i = index;
        Point vec;
        vec.x = FileReader::byteToSByte(data[++i]);
        vec.y = FileReader::byteToSByte(data[++i]);
        state.currentPoint.add(vec.multiply(state.scale));
        if (state.isPenDown)
            state.currentPolyline.push_back(state.currentPoint.clone());
        return i;
    }

    size_t ShapeParser::handleMultipleXYDisplacements(const std::vector<uint8_t>& data,
        size_t index, ParseState& state) 
    {
        size_t i = index;
        while (true) {
            Point vec;
            vec.x = FileReader::byteToSByte(data[++i]);
            vec.y = FileReader::byteToSByte(data[++i]);
            if (vec.x == 0.0 && vec.y == 0.0) break;
            state.currentPoint.add(vec.multiply(state.scale));
            if (state.isPenDown)
                state.currentPolyline.push_back(state.currentPoint.clone());
        }
        return i;
    }

    size_t ShapeParser::handleOctantArc(const std::vector<uint8_t>& data,
        size_t index, ParseState& state) 
    {
        size_t i = index;
        double radius = data[++i] * state.scale;
        int8_t flag = FileReader::byteToSByte(data[++i]);
        int startOctant = (flag & 0x70) >> 4;
        int octantCount = flag & 0x07;
        bool isClockwise = (flag < 0);
        double startRad = (M_PI / 4.0) * startOctant;

        Point center = state.currentPoint.clone().subtract(
            Point(std::cos(startRad) * radius, std::sin(startRad) * radius));

        Arc arc = Arc::fromOctant(center, radius, startOctant, octantCount, isClockwise);

        if (state.isPenDown) {
            auto pts = arc.tessellate();
            if (!state.currentPolyline.empty()) state.currentPolyline.pop_back();
            for (auto& p : pts) state.currentPolyline.push_back(p);
        }
        auto tessellated = arc.tessellate();
        if (!tessellated.empty())
            state.currentPoint = tessellated.back();
        return i;
    }

    size_t ShapeParser::handleFractionalArc(const std::vector<uint8_t>& data,
        size_t index, ParseState& state) 
    {
        size_t i = index;
        uint8_t startOffset = data[++i];
        uint8_t endOffset = data[++i];
        uint8_t hr = data[++i];
        uint8_t lr = data[++i];
        // hr * 255 + lr: faithful port of the original TypeScript formula (not 256).
        double r = (hr * 255.0 + lr) * state.scale;
        int8_t flag = FileReader::byteToSByte(data[++i]);
        int n1 = (flag & 0x70) >> 4;
        int n2 = flag & 0x07;
        if (n2 == 0) n2 = 8;
        if (endOffset != 0) --n2;

        const double pi4 = M_PI / 4.0;
        double span = pi4 * n2;
        double delta = CIRCLE_SPAN;
        double sign = 1.0;
        if (flag < 0) {
            delta = -delta;
            span = -span;
            sign = -1.0;
        }

        double startRad = pi4 * n1;
        double endRad = startRad + span;
        startRad += ((pi4 * startOffset) / 256.0) * sign;
        endRad += ((pi4 * endOffset) / 256.0) * sign;

        Point center = state.currentPoint.clone().subtract(
            Point(r * std::cos(startRad), r * std::sin(startRad)));

        state.currentPoint = center.clone().add(
            Point(r * std::cos(endRad), r * std::sin(endRad)));

        if (state.isPenDown) {
            double cur = startRad;
            std::vector<Point> pts;
            pts.push_back(center.clone().add(Point(r * std::cos(cur), r * std::sin(cur))));
            if (delta > 0.0) {
                while (cur + delta < endRad) {
                    cur += delta;
                    pts.push_back(center.clone().add(Point(r * std::cos(cur), r * std::sin(cur))));
                }
            }
            else {
                while (cur + delta > endRad) {
                    cur += delta;
                    pts.push_back(center.clone().add(Point(r * std::cos(cur), r * std::sin(cur))));
                }
            }
            pts.push_back(center.clone().add(Point(r * std::cos(endRad), r * std::sin(endRad))));
            for (auto& p : pts) state.currentPolyline.push_back(p);
        }
        return i;
    }

    size_t ShapeParser::handleBulgeArc(const std::vector<uint8_t>& data,
        size_t index, ParseState& state) 
    {
        size_t i = index;
        Point vec;
        vec.x = FileReader::byteToSByte(data[++i]);
        vec.y = FileReader::byteToSByte(data[++i]);
        int8_t bulge = FileReader::byteToSByte(data[++i]);
        state.currentPoint = handleArcSegment(
            state.currentPoint, vec, bulge, state.scale,
            state.isPenDown, state.currentPolyline);
        return i;
    }

    size_t ShapeParser::handleMultipleBulgeArcs(const std::vector<uint8_t>& data,
        size_t index, ParseState& state)
    {
        size_t i = index;
        while (true) {
            Point vec;
            vec.x = FileReader::byteToSByte(data[++i]);
            vec.y = FileReader::byteToSByte(data[++i]);
            if (vec.x == 0.0 && vec.y == 0.0) break;
            int8_t bulge = FileReader::byteToSByte(data[++i]);
            state.currentPoint = handleArcSegment(
                state.currentPoint, vec, bulge, state.scale,
                state.isPenDown, state.currentPolyline);
        }
        return i;
    }

    size_t ShapeParser::skipCode(const std::vector<uint8_t>& data, size_t index) 
    {
        uint8_t cb = data[index];
        switch (cb) {
        case 0x00: case 0x01: case 0x02: break;
        case 0x03: case 0x04: ++index; break;
        case 0x05: case 0x06: break;
        case 0x07:
            switch (fontData_.header.fontType) {
            case FontType::SHAPES:
                ++index;
                break;
            case FontType::BIGFONT: {
                ++index;
                uint8_t sc = data[index];
                if (sc == 0)
                    index += fontData_.content.isExtended ? 6 : 5;
                break;
            }
            case FontType::UNIFONT:
                index += 2;
                break;
            }
            break;
        case 0x08: index += 2; break;
        case 0x09:
            while (true) {
                uint8_t x = data[++index];
                uint8_t y = data[++index];
                if (x == 0 && y == 0) break;
            }
            break;
        case 0x0A: index += 2; break;
        case 0x0B: index += 5; break;
        case 0x0C: index += 3; break;
        case 0x0D:
            while (true) {
                uint8_t x = data[++index];
                uint8_t y = data[++index];
                if (x == 0 && y == 0) break;
                ++index;
            }
            break;
        case 0x0E: break;
        default:   break;
        }
        return index;
    }

    std::optional<ShxShape> ShapeParser::getScaledSubshapeAtInsertPoint(
        uint32_t code, double width, double height, const Point& insertPoint) 
    {

        ShxShape* base = nullptr;
        auto it = shapeCache_.find(code);
        if (it != shapeCache_.end()) {
            base = &it->second;
        }
        else {
            auto dataIt = fontData_.content.data.find(code);
            if (dataIt == fontData_.content.data.end()) return std::nullopt;
            shapeCache_.emplace(code, parseShape(dataIt->second));
            base = &shapeCache_.at(code);
        }

        ShxShape normalized = base->normalizeToOrigin(true);
        ShxShape scaled = scaleShapeByHeightAndWidth(normalized, height, width);
        return scaled.offset(insertPoint, true);
    }

    Point ShapeParser::handleArcSegment(const Point& currentPoint, Point vec,
        int8_t bulge, double scale, bool isPenDown,
        std::vector<Point>& currentPolyline) 
    {
        vec.x *= scale;
        vec.y *= scale;

        if (bulge < -127) bulge = -127;

        Point newPoint = currentPoint.clone();
        if (isPenDown) {
            if (bulge == 0) {
                currentPolyline.push_back(newPoint.clone().add(vec));
            }
            else {
                Point endPt = newPoint.clone().add(vec);
                Arc arc = Arc::fromBulge(newPoint, endPt, bulge / 127.0);
                auto pts = arc.tessellate();
                // Skip first point (currentPoint already in polyline)
                for (size_t k = 1; k < pts.size(); ++k)
                    currentPolyline.push_back(pts[k]);
            }
        }
        newPoint.add(vec);
        return newPoint;
    }

    Point ShapeParser::getVectorForDirection(int dir) const noexcept 
    {
        Point vec;
        switch (dir) {
        case 0:  vec.x = 1.0;              break;
        case 1:  vec.x = 1.0; vec.y = 0.5; break;
        case 2:  vec.x = 1.0; vec.y = 1.0; break;
        case 3:  vec.x = 0.5; vec.y = 1.0; break;
        case 4:               vec.y = 1.0; break;
        case 5:  vec.x = -0.5; vec.y = 1.0; break;
        case 6:  vec.x = -1.0; vec.y = 1.0; break;
        case 7:  vec.x = -1.0; vec.y = 0.5; break;
        case 8:  vec.x = -1.0;              break;
        case 9:  vec.x = -1.0; vec.y = -0.5; break;
        case 10: vec.x = -1.0; vec.y = -1.0; break;
        case 11: vec.x = -0.5; vec.y = -1.0; break;
        case 12:               vec.y = -1.0; break;
        case 13: vec.x = 0.5; vec.y = -1.0; break;
        case 14: vec.x = 1.0; vec.y = -1.0; break;
        case 15: vec.x = 1.0; vec.y = -0.5; break;
        default: break;
        }
        return vec;
    }

} 
