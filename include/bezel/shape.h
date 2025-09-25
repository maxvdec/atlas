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
#include <glm/glm.hpp>
#include <vector>

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

    virtual float fastestLinearSpeed(const glm::vec3 &linearVelocity,
                                     const glm::vec3 &dir) const {
        return 0.0f;
    }

    virtual void build(const std::vector<glm::vec3> points) {};

  protected:
    glm::vec3 centerOfMass = {0.0f, 0.0f, 0.0f};
};

class Sphere : public Shape {
  public:
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

class Box : public Shape {
  public:
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

class Convex : public Shape {
  public:
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

struct Triangle {
    int a;
    int b;
    int c;
};

struct Edge {
    int a;
    int b;

    inline bool operator==(const Edge &other) const {
        return (a == other.a && b == other.b) || (a == other.b && b == other.a);
    }
};

namespace bezel {
bool raySphere(const glm::vec3 &rayOrigin, const glm::vec3 &rayDirection,
               const glm::vec3 &sphereCenter, const float sphereRadius,
               float &t1, float &t2);
bool sphereToSphereDynamic(const Sphere *sphereA, const Sphere *sphereB,
                           const glm::vec3 &posA, const glm::vec3 &posB,
                           const glm::vec3 &velA, const glm::vec3 &velB,
                           const float dt, glm::vec3 &pointOnA,
                           glm::vec3 &pointOnB, float &toi);

// Shape related utilities

int findFurthestPointInDirection(const std::vector<glm::vec3> &points,
                                 const glm::vec3 &dir);
float distanceFromLine(const glm::vec3 &a, const glm::vec3 &b,
                       const glm::vec3 &pt);
glm::vec3 findFurthestPointFromLine(const std::vector<glm::vec3> &points,
                                    const glm::vec3 &a, const glm::vec3 &b);
float distanceFromTriangle(const glm::vec3 &a, const glm::vec3 &b,
                           const glm::vec3 &c, const glm::vec3 &pt);
glm::vec3 findFurthestPointFromTriangle(const std::vector<glm::vec3> &points,
                                        const glm::vec3 &a, const glm::vec3 &b,
                                        const glm::vec3 &c);
void computeTetrahedron(std::vector<glm::vec3> vert,
                        std::vector<glm::vec3> &hullPts,
                        std::vector<Triangle> &hullTris);
void expandConvexHull(std::vector<glm::vec3> &hullPts,
                      std::vector<Triangle> &hullTris,
                      const std::vector<glm::vec3> &vertices);
void removeInternalPoints(std::vector<glm::vec3> &hullPts,
                          std::vector<Triangle> &hullTris,
                          std::vector<glm::vec3> &checkPts);
bool isEdgeUnique(const std::vector<Triangle> &triangles,
                  const std::vector<int> &facingTris, const int ignoreTri,
                  const Edge &edge);
void addPoint(std::vector<glm::vec3> &hullPts, std::vector<Triangle> &hullTris,
              const glm::vec3 &pt);
void removeUnreferencedVertices(std::vector<glm::vec3> &hullPts,
                                std::vector<Triangle> &hullTris);
void buildConvexHull(std::vector<glm::vec3> points,
                     std::vector<glm::vec3> &hullPts,
                     std::vector<Triangle> &hullTris);

bool isExternal(const std::vector<glm::vec3> &pts,
                const std::vector<Triangle> &tris, const glm::vec3 &pt);
glm::vec3 calculateCenterOfMass(const std::vector<glm::vec3> &pts,
                                const std::vector<Triangle> &tris);
glm::mat3 calculateInertiaTensor(const std::vector<glm::vec3> &pts,
                                 const std::vector<Triangle> &tris,
                                 const glm::vec3 &centerOfMass);

} // namespace bezel

#endif // BEZEL_SHAPE_H
