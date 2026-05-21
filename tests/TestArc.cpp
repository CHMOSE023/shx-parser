#include <gtest/gtest.h>
#include "../src/Arc.h"
#include "../src/Point.h"
#include <cmath>

using shx::Arc;
using shx::Point;

// ─── fromBulge ────────────────────────────────────────────────────────────────

TEST(ArcFromBulge, PositiveBulgeSemicircle) {
    Arc arc = Arc::fromBulge(Point(0,0), Point(100,0), 1.0);
    EXPECT_NEAR(arc.radius, 50.0, 1e-6);
    EXPECT_FALSE(arc.isClockwise);
}

TEST(ArcFromBulge, NegativeBulgeSemicircle) {
    Arc arc = Arc::fromBulge(Point(0,0), Point(100,0), -1.0);
    EXPECT_NEAR(arc.radius, 50.0, 1e-6);
    EXPECT_TRUE(arc.isClockwise);
}

TEST(ArcFromBulge, BulgeHalf) {
    Arc arc = Arc::fromBulge(Point(0,0), Point(100,0), 0.5);
    EXPECT_NEAR(arc.radius, 62.5, 1e-4);
    EXPECT_FALSE(arc.isClockwise);
}

TEST(ArcFromBulge, QuarterCircle) {
    Arc arc = Arc::fromBulge(Point(0,0), Point(100,0), std::tan(M_PI / 8.0));
    EXPECT_NEAR(arc.radius, 70.71, 0.01);
    EXPECT_FALSE(arc.isClockwise);
}

TEST(ArcFromBulge, StraightLine) {
    Arc arc = Arc::fromBulge(Point(0,0), Point(100,0), 0.0);
    EXPECT_EQ(arc.radius, 0.0);
    auto pts = arc.tessellate();
    ASSERT_EQ(pts.size(), 2u);
    EXPECT_EQ(pts[0].x, 0.0);  EXPECT_EQ(pts[0].y, 0.0);
    EXPECT_EQ(pts[1].x, 100.0); EXPECT_EQ(pts[1].y, 0.0);
}

TEST(ArcFromBulge, ClampBulge) {
    Arc arc = Arc::fromBulge(Point(0,0), Point(100,0), 2.0); // clamped to 1
    EXPECT_NEAR(arc.radius, 50.0, 1e-6);
    EXPECT_FALSE(arc.isClockwise);
}

// ─── fromOctant ───────────────────────────────────────────────────────────────

TEST(ArcFromOctant, CounterClockwise) {
    Arc arc = Arc::fromOctant(Point(0,0), 5.0, 0, 2, false);
    EXPECT_EQ(arc.radius, 5.0);
    EXPECT_FALSE(arc.isClockwise);
    EXPECT_NEAR(arc.startAngle, 0.0, 1e-9);
    EXPECT_NEAR(arc.endAngle,   M_PI / 2.0, 1e-9);
    EXPECT_NEAR(arc.start.x, 5.0, 1e-9);
    EXPECT_NEAR(arc.start.y, 0.0, 1e-9);
    EXPECT_NEAR(arc.end.x,   0.0, 1e-9);
    EXPECT_NEAR(arc.end.y,   5.0, 1e-9);
}

TEST(ArcFromOctant, Clockwise) {
    Arc arc = Arc::fromOctant(Point(0,0), 3.0, 2, 3, true);
    EXPECT_EQ(arc.radius, 3.0);
    EXPECT_TRUE(arc.isClockwise);
    EXPECT_NEAR(arc.startAngle, M_PI / 2.0, 1e-9);
    EXPECT_NEAR(arc.endAngle,  -M_PI / 4.0, 1e-9);
}

TEST(ArcFromOctant, FullCircle) {
    Arc arc = Arc::fromOctant(Point(1,1), 2.0, 4, 0, false);
    EXPECT_EQ(arc.radius, 2.0);
    EXPECT_NEAR(arc.startAngle, M_PI, 1e-9);
    EXPECT_NEAR(arc.endAngle,   M_PI + 2.0 * M_PI, 1e-9);
    EXPECT_NEAR(arc.start.x, 1.0 - 2.0, 1e-9);
    EXPECT_NEAR(arc.start.y, 1.0,        1e-9);
}

TEST(ArcFromOctant, MultipleOctants) {
    Arc arc = Arc::fromOctant(Point(0,0), 4.0, 1, 5, false);
    EXPECT_NEAR(arc.startAngle, M_PI / 4.0, 1e-9);
    EXPECT_NEAR(arc.endAngle, 3.0 * M_PI / 2.0, 1e-9);
    EXPECT_NEAR(arc.start.x, 4.0 * std::cos(M_PI / 4.0), 1e-9);
    EXPECT_NEAR(arc.start.y, 4.0 * std::sin(M_PI / 4.0), 1e-9);
    EXPECT_NEAR(arc.end.x,   0.0,  1e-9);
    EXPECT_NEAR(arc.end.y,  -4.0,  1e-9);
}

// ─── tessellate ───────────────────────────────────────────────────────────────

static constexpr double TEST_SPAN = M_PI / 16.0;

TEST(ArcTessellate, Semicircle) {
    Arc arc = Arc::fromBulge(Point(0,0), Point(100,0), 1.0);
    auto pts = arc.tessellate(TEST_SPAN);
    EXPECT_EQ(pts.size(), 17u);
    EXPECT_NEAR(pts.front().x, 0.0,   1e-4);
    EXPECT_NEAR(pts.front().y, 0.0,   1e-4);
    EXPECT_NEAR(pts.back().x,  100.0, 1e-4);
    EXPECT_NEAR(pts.back().y,  0.0,   1e-4);
    // midpoint at 90° (index 8)
    EXPECT_NEAR(pts[8].x,  50.0, 1e-4);
    EXPECT_NEAR(pts[8].y, -50.0, 1e-4);
}

TEST(ArcTessellate, ClockwiseSemicircle) {
    Arc arc = Arc::fromBulge(Point(0,0), Point(100,0), -1.0);
    auto pts = arc.tessellate(TEST_SPAN);
    EXPECT_EQ(pts.size(), 17u);
    EXPECT_NEAR(pts[8].x,  50.0, 1e-4);
    EXPECT_NEAR(pts[8].y,  50.0, 1e-4);
}

TEST(ArcTessellate, QuarterCircle) {
    Arc arc = Arc::fromBulge(Point(0,0), Point(100,0), std::tan(M_PI / 8.0));
    auto pts = arc.tessellate(TEST_SPAN);
    EXPECT_EQ(pts.size(), 9u);
}

TEST(ArcTessellate, OctantArc) {
    Arc arc = Arc::fromOctant(Point(0,0), 100.0, 0, 2, false);
    auto pts = arc.tessellate(TEST_SPAN);
    EXPECT_EQ(pts.size(), 9u);
    EXPECT_NEAR(pts[0].x, 100.0, 1e-4);
    EXPECT_NEAR(pts[0].y,   0.0, 1e-4);
    EXPECT_NEAR(pts[4].x, 100.0 * std::cos(M_PI / 4.0), 1e-4);
    EXPECT_NEAR(pts[4].y, 100.0 * std::sin(M_PI / 4.0), 1e-4);
    EXPECT_NEAR(pts.back().x, 0.0,   1e-4);
    EXPECT_NEAR(pts.back().y, 100.0, 1e-4);
}

TEST(ArcTessellate, StraightLine) {
    Arc arc = Arc::fromBulge(Point(0,0), Point(100,0), 0.0);
    auto pts = arc.tessellate(TEST_SPAN);
    ASSERT_EQ(pts.size(), 2u);
    EXPECT_EQ(pts[0].x,   0.0);
    EXPECT_EQ(pts[1].x, 100.0);
}
