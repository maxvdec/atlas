/*
 bounds.h
 As part of the Atlas project
 Created by Max Van den Eynde in 2025
 --------------------------------------------------
 Description: Functions for bound-checking algorithms
 Copyright (c) 2025 maxvdec
*/

#ifndef BEZEL_BOUNDS_H
#define BEZEL_BOUNDS_H

#include <glm/glm.hpp>
#include <memory>
#include <vector>

class Bounds {
  public:
    Bounds();
    Bounds(const Bounds &rhs) : mins(rhs.mins), maxs(rhs.maxs) {}
    const Bounds &operator=(const Bounds &rhs);
    ~Bounds() = default;

    void clear();
    bool doesIntersect(const Bounds &other);
    void expand(std::vector<glm::vec3> pts, const int number);
    void expand(const glm::vec3 rhs);
    void expand(const Bounds &rhs);

    inline float widthX() const { return maxs.x - mins.x; }
    inline float widthY() const { return maxs.y - mins.y; }
    inline float widthZ() const { return maxs.z - mins.z; }

    glm::vec3 mins;
    glm::vec3 maxs;
};

struct PseudoBody;
class Body;

struct CollisionPair {
    int a;
    int b;

    inline bool operator==(const CollisionPair &other) const {
        return (a == other.a && b == other.b) || (a == other.b && b == other.a);
    }

    inline bool operator!=(const CollisionPair &other) const {
        return !(*this == other);
    }
};

namespace bezel {
int compareSAP(void *a, void *b);

void sortBodiesForBounds(std::vector<std::shared_ptr<Body>> bodies,
                         std::vector<PseudoBody> &sortedArray, float dt);

void buildPairs(std::vector<CollisionPair> &pairs,
                std::vector<PseudoBody> &bodies);

void sweepAndPrune1d(std::vector<std::shared_ptr<Body>> bodies,
                     std::vector<CollisionPair> &pairs, float dt);

void broadPhase(std::vector<std::shared_ptr<Body>> bodies,
                std::vector<CollisionPair> &pairs, float dt);
} // namespace bezel

#endif // BEZEL_BOUNDS_H
