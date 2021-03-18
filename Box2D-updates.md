Box 2.4.1:
  * b2_common.h: `b2_maxTranslation` changed same as in 2.3.1.
  * b2PolygonShape: Same changes as in 2.3.1

Box 2.3.1:
  * CMake: Unrelevant changes.
  * b2PrismaticJoint.cpp: commented out s1test in `b2PrismaticJoint::InitVelocityConstraints`
  * b2Settings.h: `b2_maxTranslation` bumped from `2.f` to `20.f`
  * b2Math.h: non-functional change in `b2IsValid()` (one extra intermediate variable)
  * b2GrowableStack.h: added `<string.h>` include, allegedly to fix an Android build.
  * b2PolygonShape.h: Changed `Set(const b2Vec2* points, int32 count);`, returns a boolean.
  *    "          .cpp:
        - `b2Vec2 ComputeCentroid()`: commented out `b2Assert(area > b2_epsilon);`
        - `b2PolygonShape::Set()`: asserts have been commented out, and converted to return values.
                                   if() adapted where necessary.
        - `b2PolygonShape::ComputeMass()`: commented out `b2Assert(area > b2_epsilon);`