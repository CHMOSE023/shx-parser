#include <gtest/gtest.h>
#include "../src/Point.h"

using shx::Point;

TEST(PointConstructor, DefaultValues) {
    Point p;
    EXPECT_EQ(p.x, 0.0);
    EXPECT_EQ(p.y, 0.0);
}

TEST(PointConstructor, SpecifiedValues) {
    Point p(3.0, 4.0);
    EXPECT_EQ(p.x, 3.0);
    EXPECT_EQ(p.y, 4.0);
}

TEST(PointSet, SetsCoordinates) {
    Point p;
    p.set(3.0, 4.0);
    EXPECT_EQ(p.x, 3.0);
    EXPECT_EQ(p.y, 4.0);
}

TEST(PointSet, ReturnsInstance) {
    Point p;
    Point& result = p.set(3.0, 4.0);
    EXPECT_EQ(&result, &p);
}

TEST(PointLength, CorrectLength) {
    Point p(3.0, 4.0);
    EXPECT_EQ(p.length(), 5.0);
}

TEST(PointLength, ZeroForOrigin) {
    Point p;
    EXPECT_EQ(p.length(), 0.0);
}

TEST(PointNormalize, UnitLength) {
    Point p(3.0, 4.0);
    p.normalize();
    EXPECT_NEAR(p.x, 0.6, 1e-9);
    EXPECT_NEAR(p.y, 0.8, 1e-9);
    EXPECT_NEAR(p.length(), 1.0, 1e-9);
}

TEST(PointNormalize, ZeroVector) {
    Point p;
    p.normalize();
    EXPECT_EQ(p.x, 0.0);
    EXPECT_EQ(p.y, 0.0);
}

TEST(PointClone, IndependentCopy) {
    Point p(3.0, 4.0);
    Point c = p.clone();
    EXPECT_EQ(c.x, p.x);
    EXPECT_EQ(c.y, p.y);
    c.x = 99.0;
    EXPECT_EQ(p.x, 3.0); // original unchanged
}

TEST(PointAdd, AddsCoordinates) {
    Point p1(1.0, 2.0);
    Point p2(3.0, 4.0);
    p1.add(p2);
    EXPECT_EQ(p1.x, 4.0);
    EXPECT_EQ(p1.y, 6.0);
}

TEST(PointSubtract, SubtractsCoordinates) {
    Point p1(3.0, 4.0);
    Point p2(1.0, 2.0);
    p1.subtract(p2);
    EXPECT_EQ(p1.x, 2.0);
    EXPECT_EQ(p1.y, 2.0);
}

TEST(PointMultiply, ScalarMultiply) {
    Point p(2.0, 3.0);
    p.multiply(2.0);
    EXPECT_EQ(p.x, 4.0);
    EXPECT_EQ(p.y, 6.0);
}

TEST(PointDivide, ScalarDivide) {
    Point p(4.0, 6.0);
    p.divide(2.0);
    EXPECT_EQ(p.x, 2.0);
    EXPECT_EQ(p.y, 3.0);
}

TEST(PointDivide, DivideByZero) {
    Point p(4.0, 6.0);
    p.divide(0.0);
    EXPECT_EQ(p.x, 4.0);
    EXPECT_EQ(p.y, 6.0);
}

TEST(PointMultiplyScalars, DifferentScalars) {
    Point p(2.0, 3.0);
    p.multiplyScalars(2.0, 3.0);
    EXPECT_EQ(p.x, 4.0);
    EXPECT_EQ(p.y, 9.0);
}

TEST(PointMultiplyScalars, ReturnsInstance) {
    Point p(2.0, 3.0);
    Point& r = p.multiplyScalars(2.0, 3.0);
    EXPECT_EQ(&r, &p);
}

TEST(PointDivideScalars, DifferentScalars) {
    Point p(10.0, 15.0);
    p.divideScalars(2.0, 3.0);
    EXPECT_EQ(p.x, 5.0);
    EXPECT_EQ(p.y, 5.0);
}

TEST(PointDivideScalars, DivideXByZero) {
    Point p(10.0, 15.0);
    p.divideScalars(0.0, 3.0);
    EXPECT_EQ(p.x, 10.0); // unchanged
    EXPECT_EQ(p.y, 5.0);
}

TEST(PointDivideScalars, ReturnsInstance) {
    Point p(10.0, 15.0);
    Point& r = p.divideScalars(2.0, 3.0);
    EXPECT_EQ(&r, &p);
}

TEST(PointDistanceTo, CorrectDistance) {
    Point p1(0.0, 0.0);
    Point p2(3.0, 4.0);
    EXPECT_EQ(p1.distanceTo(p2), 5.0);
}

TEST(PointDistanceTo, SamePoint) {
    Point p1(3.0, 4.0);
    Point p2(3.0, 4.0);
    EXPECT_EQ(p1.distanceTo(p2), 0.0);
}
