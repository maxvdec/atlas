/*
 shape.h
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Simple shape functions and defintions
 Copyright (c) 2025 maxvdec
*/

#ifndef BEZEL_SHAPE_H
#define BEZEL_SHAPE_H

#include "bezel/bounds.h"
#include <array>
#include <glm/glm.hpp>
#include <memory>
#include <vector>

/**
 * @brief Abstract base class for all collision shapes in the physics engine.
 * Provides interface for collision detection and physics calculations.
 *
 */
class Shape {
  public:
    enum class ShapeType { Sphere, Box, Convex };

    virtual ShapeType getType() const = 0;

    virtual glm::vec3 getCenterOfMass() const { return centerOfMass; }

    virtual glm::mat3 getInertiaTensor() const = 0;

    virtual Bounds getBounds(const glm::vec3 &pos,
                             const glm::quat &orientation) const = 0;
    virtual Bounds getBounds() const = 0;

    virtual glm::vec3 support(const glm::vec3 &dir, const glm::vec3 &pos,
                              const glm::quat &orientation,
                              float bias) const = 0;

    virtual float fastestLinearSpeed(const glm::vec3 &,
                                     const glm::vec3 &) const {
        return 0.0f;
    }

    virtual void build(const std::vector<glm::vec3>) {};

    virtual ~Shape() = default;

  protected:
    glm::vec3 centerOfMass = {0.0f, 0.0f, 0.0f};
};

/**
 * @brief Spherical collision shape with a defined radius.
 *
 */
class Sphere : public Shape {
  public:
    /**
     * @brief Constructs a sphere with the specified radius.
     *
     * @param radius The radius of the sphere.
     */
    explicit Sphere(float radius);
    ShapeType getType() const override { return ShapeType::Sphere; }

    glm::mat3 getInertiaTensor() const override;

    Bounds getBounds(const glm::vec3 &pos,
                     const glm::quat &orientation) const override;
    Bounds getBounds() const override;

    glm::vec3 support(const glm::vec3 &dir, const glm::vec3 &pos,
                      const glm::quat &orientation, float bias) const override;

    float radius;
};

/**
 * @brief Box collision shape defined by a set of vertices.
 *
 */
class Box : public Shape {
  public:
    /**
     * @brief Constructs a box from a set of points.
     *
     * @param points The vertices defining the box shape.
     */
    explicit Box(std::vector<glm::vec3> points);

    void build(const std::vector<glm::vec3> points) override;

    glm::vec3 support(const glm::vec3 &dir, const glm::vec3 &pos,
                      const glm::quat &orientation, float bias) const override;

    glm::mat3 getInertiaTensor() const override;

    Bounds getBounds(const glm::vec3 &pos,
                     const glm::quat &orientation) const override;
    Bounds getBounds() const override;

    float fastestLinearSpeed(const glm::vec3 &angularVelocity,
                             const glm::vec3 &dir) const override;

    ShapeType getType() const override { return ShapeType::Box; }

    std::vector<glm::vec3> vertices;
    Bounds bounds;
};

/**
 * @brief Convex hull collision shape built from a set of points.
 *
 */
class Convex : public Shape {
  public:
    /**
     * @brief Constructs a convex hull from a set of points.
     *
     * @param points The vertices to build the convex hull from.
     */
    explicit Convex(std::vector<glm::vec3> points);

    void build(const std::vector<glm::vec3> points) override;

    glm::vec3 support(const glm::vec3 &dir, const glm::vec3 &pos,
                      const glm::quat &orientation, float bias) const override;

    glm::mat3 getInertiaTensor() const override;

    Bounds getBounds(const glm::vec3 &pos,
                     const glm::quat &orientation) const override;
    Bounds getBounds() const override;

    float fastestLinearSpeed(const glm::vec3 &angularVelocity,
                             const glm::vec3 &dir) const override;

    ShapeType getType() const override { return ShapeType::Convex; }

    std::vector<glm::vec3> vertices;
    Bounds bounds;
    glm::mat3 inertiaTensor;
};

/**
 * @brief Structure representing a triangle with three vertex indices.
 *
 */
struct Triangle {
    int a;
    int b;
    int c;
};

/**
 * @brief Structure representing an edge between two vertices.
 *
 */
struct Edge {
    int a;
    int b;

    inline bool operator==(const Edge &other) const {
        return (a == other.a && b == other.b) || (a == other.b && b == other.a);
    }
};

/**
 * @brief Structure representing a point in 3D space with additional data for
 * collision detection.
 *
 */
struct Point {
    glm::vec3 xyz;
    glm::vec3 ptA;
    glm::vec3 ptB;

    Point() : xyz(0.0f), ptA(0.0f), ptB(0.0f) {}

    Point(const Point &other)
        : xyz(other.xyz), ptA(other.ptA), ptB(other.ptB) {}

    inline const Point &operator=(const Point &other) {
        xyz = other.xyz;
        ptA = other.ptA;
        ptB = other.ptB;
        return *this;
    }

    inline bool operator==(const Point &other) const {
        return glm::all(glm::equal(xyz, other.xyz)) &&
               glm::all(glm::equal(ptA, other.ptA)) &&
               glm::all(glm::equal(ptB, other.ptB));
    }
};

/**
 * @brief Namespace containing collision detection algorithms and shape
 * utilities.
 *
 */
namespace bezel {
/**
 * @brief Tests intersection between a ray and a sphere.
 *
 * @param rayOrigin The origin point of the ray.
 * @param rayDirection The direction vector of the ray.
 * @param sphereCenter The center point of the sphere.
 * @param sphereRadius The radius of the sphere.
 * @param t1 Output parameter for the first intersection distance.
 * @param t2 Output parameter for the second intersection distance.
 * @return (bool) True if intersection occurs, false otherwise.
 */
bool raySphere(const glm::vec3 &rayOrigin, const glm::vec3 &rayDirection,
               const glm::vec3 &sphereCenter, const float sphereRadius,
               float &t1, float &t2);
/**
 * @brief Tests dynamic collision between two moving spheres.
 *
 * @param sphereA The first sphere.
 * @param sphereB The second sphere.
 * @param posA Position of the first sphere.
 * @param posB Position of the second sphere.
 * @param velA Velocity of the first sphere.
 * @param velB Velocity of the second sphere.
 * @param dt Delta time for the collision test.
 * @param pointOnA Output point on the first sphere.
 * @param pointOnB Output point on the second sphere.
 * @param toi Output time of impact.
 * @return (bool) True if collision occurs, false otherwise.
 */
bool sphereToSphereDynamic(const Sphere *sphereA, const Sphere *sphereB,
                           const glm::vec3 &posA, const glm::vec3 &posB,
                           const glm::vec3 &velA, const glm::vec3 &velB,
                           const float dt, glm::vec3 &pointOnA,
                           glm::vec3 &pointOnB, float &toi);

// Shape related utilities

/**
 * @brief Finds the point furthest in a given direction.
 *
 * @param points Vector of points to search through.
 * @param dir Direction vector to search along.
 * @return (int) Index of the furthest point.
 */
int findFurthestPointInDirection(const std::vector<glm::vec3> &points,
                                 const glm::vec3 &dir);
/**
 * @brief Calculates distance from a point to a line.
 *
 * @param a First point on the line.
 * @param b Second point on the line.
 * @param pt Point to measure distance from.
 * @return (float) Distance from point to line.
 */
float distanceFromLine(const glm::vec3 &a, const glm::vec3 &b,
                       const glm::vec3 &pt);
/**
 * @brief Finds the point furthest from a line.
 *
 * @param points Vector of points to search through.
 * @param a First point on the line.
 * @param b Second point on the line.
 * @return (glm::vec3) The furthest point from the line.
 */
glm::vec3 findFurthestPointFromLine(const std::vector<glm::vec3> &points,
                                    const glm::vec3 &a, const glm::vec3 &b);
/**
 * @brief Calculates distance from a point to a triangle.
 *
 * @param a First vertex of the triangle.
 * @param b Second vertex of the triangle.
 * @param c Third vertex of the triangle.
 * @param pt Point to measure distance from.
 * @return (float) Distance from point to triangle.
 */
float distanceFromTriangle(const glm::vec3 &a, const glm::vec3 &b,
                           const glm::vec3 &c, const glm::vec3 &pt);
/**
 * @brief Finds the point furthest from a triangle.
 *
 * @param points Vector of points to search through.
 * @param a First vertex of the triangle.
 * @param b Second vertex of the triangle.
 * @param c Third vertex of the triangle.
 * @return (glm::vec3) The furthest point from the triangle.
 */
glm::vec3 findFurthestPointFromTriangle(const std::vector<glm::vec3> &points,
                                        const glm::vec3 &a, const glm::vec3 &b,
                                        const glm::vec3 &c);
/**
 * @brief Computes an initial tetrahedron for convex hull construction.
 *
 * @param vert Input vertices.
 * @param hullPts Output hull points.
 * @param hullTris Output hull triangles.
 */
void computeTetrahedron(std::vector<glm::vec3> vert,
                        std::vector<glm::vec3> &hullPts,
                        std::vector<Triangle> &hullTris);
/**
 * @brief Expands an existing convex hull with additional vertices.
 *
 * @param hullPts Current hull points to expand.
 * @param hullTris Current hull triangles to expand.
 * @param vertices Additional vertices to include.
 */
void expandConvexHull(std::vector<glm::vec3> &hullPts,
                      std::vector<Triangle> &hullTris,
                      const std::vector<glm::vec3> &vertices);
/**
 * @brief Removes internal points from a convex hull.
 *
 * @param hullPts Hull points to check.
 * @param hullTris Hull triangles for reference.
 * @param checkPts Points to check for removal.
 */
void removeInternalPoints(std::vector<glm::vec3> &hullPts,
                          std::vector<Triangle> &hullTris,
                          std::vector<glm::vec3> &checkPts);
/**
 * @brief Checks if an edge is unique among triangles.
 *
 * @param triangles Vector of triangles to check.
 * @param facingTris Indices of triangles facing a point.
 * @param ignoreTri Triangle index to ignore.
 * @param edge Edge to check for uniqueness.
 * @return (bool) True if edge is unique, false otherwise.
 */
bool isEdgeUnique(const std::vector<Triangle> &triangles,
                  const std::vector<int> &facingTris, const int ignoreTri,
                  const Edge &edge);
/**
 * @brief Adds a point to an existing convex hull.
 *
 * @param hullPts Hull points to modify.
 * @param hullTris Hull triangles to modify.
 * @param pt Point to add to the hull.
 */
void addPoint(std::vector<glm::vec3> &hullPts, std::vector<Triangle> &hullTris,
              const glm::vec3 &pt);
/**
 * @brief Removes vertices that are not referenced by any triangle.
 *
 * @param hullPts Hull points to clean up.
 * @param hullTris Hull triangles for reference.
 */
void removeUnreferencedVertices(std::vector<glm::vec3> &hullPts,
                                std::vector<Triangle> &hullTris);
/**
 * @brief Builds a complete convex hull from a set of points.
 *
 * @param points Input points to build hull from.
 * @param hullPts Output hull points.
 * @param hullTris Output hull triangles.
 */
void buildConvexHull(std::vector<glm::vec3> points,
                     std::vector<glm::vec3> &hullPts,
                     std::vector<Triangle> &hullTris);

/**
 * @brief Checks if a point is external to a convex hull.
 *
 * @param pts Hull points.
 * @param tris Hull triangles.
 * @param pt Point to test.
 * @return (bool) True if point is external, false otherwise.
 */
bool isExternal(const std::vector<glm::vec3> &pts,
                const std::vector<Triangle> &tris, const glm::vec3 &pt);
/**
 * @brief Calculates the center of mass for a convex hull.
 *
 * @param pts Hull points.
 * @param tris Hull triangles.
 * @return (glm::vec3) The center of mass.
 */
glm::vec3 calculateCenterOfMass(const std::vector<glm::vec3> &pts,
                                const std::vector<Triangle> &tris);
/**
 * @brief Calculates the inertia tensor for a convex hull.
 *
 * @param pts Hull points.
 * @param tris Hull triangles.
 * @param centerOfMass The center of mass of the hull.
 * @return (glm::mat3) The inertia tensor matrix.
 */
glm::mat3 calculateInertiaTensor(const std::vector<glm::vec3> &pts,
                                 const std::vector<Triangle> &tris,
                                 const glm::vec3 &centerOfMass);

/**
 * @brief Projects points onto a 1D line.
 *
 * @param s1 First support point.
 * @param s2 Second support point.
 * @return (glm::vec2) Projection result.
 */
glm::vec2 projectOn1D(const glm::vec3 &s1, const glm::vec3 &s2);
/**
 * @brief Compares the signs of two floating point values.
 *
 * @param a First value.
 * @param b Second value.
 * @return (int) Comparison result.
 */
int compareSigns(float a, float b);
/**
 * @brief Projects points onto a 2D plane.
 *
 * @param s1 First support point.
 * @param s2 Second support point.
 * @param s3 Third support point.
 * @return (glm::vec3) Projection result.
 */
glm::vec3 projectOn2D(const glm::vec3 &s1, const glm::vec3 &s2,
                      const glm::vec3 &s3);
/**
 * @brief Projects points onto 3D space.
 *
 * @param s1 First support point.
 * @param s2 Second support point.
 * @param s3 Third support point.
 * @param s4 Fourth support point.
 * @return (glm::vec4) Projection result.
 */
glm::vec4 projectOn3D(const glm::vec3 &s1, const glm::vec3 &s2,
                      const glm::vec3 &s3, const glm::vec3 &s4);

/**
 * @brief Computes a cofactor from a 4x4 matrix.
 *
 * @param m The matrix to compute cofactor from.
 * @param row Row index.
 * @param col Column index.
 * @return (float) The cofactor value.
 */
inline float takeCofactor(const glm::mat4 &m, int row, int col) {
    glm::mat3 minor;
    int r = 0;
    for (int i = 0; i < 4; ++i) {
        if (i == row)
            continue;
        int c = 0;
        for (int j = 0; j < 4; ++j) {
            if (j == col)
                continue;
            minor[r][c] = m[i][j];
            ++c;
        }
        ++r;
    }
    float det = glm::determinant(minor);
    return ((row + col) % 2 == 0 ? 1.0f : -1.0f) * det;
}

/**
 * @brief Computes two orthogonal vectors from a given vector.
 *
 * @param v Input vector.
 * @param u Output first orthogonal vector.
 * @param w Output second orthogonal vector.
 */
inline void takeOrtho(const glm::vec3 &v, glm::vec3 &u, glm::vec3 &w) {
    glm::vec3 n = glm::normalize(v);

    glm::vec3 axis;
    if (std::abs(n.x) < std::abs(n.y) && std::abs(n.x) < std::abs(n.z)) {
        axis = glm::vec3(1.0f, 0.0f, 0.0f);
    } else if (std::abs(n.y) < std::abs(n.z)) {
        axis = glm::vec3(0.0f, 1.0f, 0.0f);
    } else {
        axis = glm::vec3(0.0f, 0.0f, 1.0f);
    }

    u = glm::normalize(glm::cross(n, axis));
    w = glm::normalize(glm::cross(n, u));
}

} // namespace bezel

namespace bezel {
// GJK functions
/**
 * @brief Computes signed volumes for simplex in GJK algorithm.
 *
 * @param simplex Current simplex points.
 * @param newDir Output new search direction.
 * @param lambdas Output barycentric coordinates.
 * @return (bool) True if origin is contained, false otherwise.
 */
bool simplexSignedVolumes(const std::vector<Point> &simplex, glm::vec3 &newDir,
                          glm::vec4 &lambdas);
/**
 * @brief Checks if a simplex contains a specific point.
 *
 * @param simplex The simplex to check.
 * @param p The point to search for.
 * @return (bool) True if point is in simplex, false otherwise.
 */
bool hasPoint(const std::array<Point, 4> &simplex, const Point &p);
/**
 * @brief Checks if a point exists in triangulated surface.
 *
 * @param w The point to search for.
 * @param triangles Surface triangles.
 * @param points Points defining the surface.
 * @return (bool) True if point exists, false otherwise.
 */
bool hasPoint(const glm::vec3 &w, const std::vector<Triangle> &triangles,
              const std::vector<Point> &points);

/**
 * @brief Counts the number of valid points in a simplex based on lambda values.
 *
 * @param lambdas Barycentric coordinates.
 * @return (int) Number of valid points.
 */
int numValids(const glm::vec4 &lambdas);
/**
 * @brief Sorts valid points in a simplex based on lambda values.
 *
 * @param simplex Simplex to sort.
 * @param lambdas Barycentric coordinates for sorting.
 */
void sortValids(std::array<Point, 4> &simplex, glm::vec4 &lambdas);
/**
 * @brief Performs GJK intersection test between two bodies.
 *
 * @param bodyA First body.
 * @param bodyB Second body.
 * @param bias Collision bias value.
 * @param ptOnA Output closest point on body A.
 * @param ptOnB Output closest point on body B.
 * @return (bool) True if intersection detected, false otherwise.
 */
bool gjkIntersection(const std::shared_ptr<Body> bodyA,
                     const std::shared_ptr<Body> bodyB, const float bias,
                     glm::vec3 &ptOnA, glm::vec3 &ptOnB, glm::vec3 &normalOut,
                     float &penetrationDepthOut);
/**
 * @brief Finds closest points between two bodies using GJK.
 *
 * @param bodyA First body.
 * @param bodyB Second body.
 * @param ptOnA Output closest point on body A.
 * @param ptOnB Output closest point on body B.
 */
void gjkClosestPoints(const std::shared_ptr<Body> bodyA,
                      const std::shared_ptr<Body> bodyB, glm::vec3 &ptOnA,
                      glm::vec3 &ptOnB);
} // namespace bezel

namespace bezel {
// EPA functions
/**
 * @brief Calculates barycentric coordinates for a point relative to a triangle.
 *
 * @param s1 First triangle vertex.
 * @param s2 Second triangle vertex.
 * @param s3 Third triangle vertex.
 * @param pt Point to calculate coordinates for.
 * @return (glm::vec3) Barycentric coordinates.
 */
glm::vec3 barycentricCoordinates(glm::vec3 &s1, glm::vec3 &s2, glm::vec3 &s3,
                                 const glm::vec3 &pt);
/**
 * @brief Computes the normal direction of a triangle.
 *
 * @param tri Triangle to compute normal for.
 * @param points Points defining the triangle vertices.
 * @return (glm::vec3) Normal direction vector.
 */
glm::vec3 normalDirection(const Triangle &tri,
                          const std::vector<Point> &points);
/**
 * @brief Calculates signed distance from a point to a triangle.
 *
 * @param tri Triangle to measure distance to.
 * @param pt Point to measure distance from.
 * @param points Points defining the triangle vertices.
 * @return (float) Signed distance value.
 */
float signedDistanceToTriangle(const Triangle &tri, const glm::vec3 &pt,
                               const std::vector<Point> &points);
/**
 * @brief Finds the triangle closest to the origin.
 *
 * @param triangles Vector of triangles to search.
 * @param points Points defining the triangle vertices.
 * @return (int) Index of the closest triangle.
 */
int closestTriangle(const std::vector<Triangle> &triangles,
                    const std::vector<Point> &points);
/**
 * @brief Removes triangles that face towards a given point.
 *
 * @param pt Point to test triangle facing.
 * @param triangles Vector of triangles to filter.
 * @param points Points defining the triangle vertices.
 * @return (int) Number of triangles removed.
 */
int removeTrianglesFacingPoint(const glm::vec3 &pt,
                               std::vector<Triangle> &triangles,
                               const std::vector<Point> &points);
/**
 * @brief Finds edges that are not shared between triangles.
 *
 * @param danglingEdges Output vector of dangling edges.
 * @param triangles Vector of triangles to analyze.
 */
void findDanglingEdges(std::vector<Edge> &danglingEdges,
                       const std::vector<Triangle> &triangles);
/**
 * @brief Expands collision detection using EPA algorithm.
 *
 * @param bodyA First body.
 * @param bodyB Second body.
 * @param bias Collision bias value.
 * @param simplex Initial simplex from GJK.
 * @param ptOnA Output closest point on body A.
 * @param ptOnB Output closest point on body B.
 * @return (float) Penetration depth.
 */
float epaExpand(const std::shared_ptr<Body> bodyA,
                const std::shared_ptr<Body> bodyB, const float bias,
                const std::array<Point, 4> simplex, glm::vec3 &ptOnA,
                glm::vec3 &ptOnB, glm::vec3 &normalOut);
} // namespace bezel

struct Contact;

/**
 * @brief Namespace containing specific collision detection functions.
 *
 */
namespace bezel::collisions {
/**
 * @brief Tests static collision between two spheres.
 *
 * @param sphereA First sphere.
 * @param sphereB Second sphere.
 * @param posA Position of first sphere.
 * @param posB Position of second sphere.
 * @param pointOnA Output contact point on first sphere.
 * @param pointOnB Output contact point on second sphere.
 * @return (bool) True if collision detected, false otherwise.
 */
bool sphereToSphereStatic(const Sphere *sphereA, const Sphere *sphereB,
                          const glm::vec3 &posA, const glm::vec3 &posB,
                          glm::vec3 &pointOnA, glm::vec3 &pointOnB);
/**
 * @brief Performs conservative advancement for continuous collision detection.
 *
 * @param bodyA First body.
 * @param bodyB Second body.
 * @param dt Delta time.
 * @param contact Output contact information.
 * @return (bool) True if collision will occur, false otherwise.
 */
bool conservativeAdvance(std::shared_ptr<Body> bodyA,
                         std::shared_ptr<Body> bodyB, float dt,
                         Contact &contact);
} // namespace bezel::collisions

#endif // BEZEL_SHAPE_H
