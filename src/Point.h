#pragma once
#include <cmath>

namespace shx
{
    class Point 
    {
    public:
        double x{ 0.0 };
        double y{ 0.0 };

        Point() = default;
        Point(double x, double y) noexcept : x(x), y(y) {}

        Point& set(double x_, double y_) noexcept 
        {
            x = x_; y = y_; return *this;
        }

        double length() const noexcept 
        {
            return std::sqrt(x * x + y * y);
        }

        Point& normalize() noexcept 
        {
            double len = length();
            if (len != 0.0) { x /= len; y /= len; }
            return *this;
        }

        Point clone() const noexcept { return Point(x, y); }

        Point& add(const Point& p) noexcept 
        {
            x += p.x; y += p.y; return *this;
        }

        Point& subtract(const Point& p) noexcept 
        {
            x -= p.x; y -= p.y; return *this;
        }

        Point& multiply(double scalar) noexcept
        {
            x *= scalar; y *= scalar; return *this;
        }

        Point& divide(double scalar) noexcept 
        {
            if (scalar != 0.0) { x /= scalar; y /= scalar; }
            return *this;
        }

        Point& multiplyScalars(double xScalar, double yScalar) noexcept
        {
            x *= xScalar; y *= yScalar; return *this;
        }

        Point& divideScalars(double xScalar, double yScalar) noexcept 
        {
            if (xScalar != 0.0) x /= xScalar;
            if (yScalar != 0.0) y /= yScalar;
            return *this;
        }

        double distanceTo(const Point& p) const noexcept 
        {
            double dx = x - p.x, dy = y - p.y;
            return std::sqrt(dx * dx + dy * dy);
        }
    };

} 
