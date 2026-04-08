#ifndef BEZIERTUBE_H
#define BEZIERTUBE_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <vector>
#include <cmath>
#include "BezierCurve.h"

class BezierTube {
public:
    static void generate(
        const glm::vec3& P0, const glm::vec3& P1,
        const glm::vec3& P2, const glm::vec3& P3,
        float radius,            // cross-section radius
        int curveSegments,       // segments along curve (e.g. 32)
        int crossSections,       // sides of the polygon (e.g. 8)
        std::vector<float>& outVertices,
        std::vector<unsigned int>& outIndices)
    {
        outVertices.clear();
        outIndices.clear();

        // Sample points along the Bezier curve
        std::vector<glm::vec3> points;
        std::vector<glm::vec3> tangents;

        for (int i = 0; i <= curveSegments; i++) {
            float t = (float)i / (float)curveSegments;
            points.push_back(BezierCurve::evaluateCubic(P0, P1, P2, P3, t));
        }

        // Compute tangents using finite differences
        for (int i = 0; i <= curveSegments; i++) {
            glm::vec3 tang;
            if (i == 0)
                tang = points[1] - points[0];
            else if (i == curveSegments)
                tang = points[curveSegments] - points[curveSegments - 1];
            else
                tang = points[i + 1] - points[i - 1];
            tangents.push_back(glm::normalize(tang));
        }

        // Build Frenet-like frames along the curve
        // Use a reference vector to avoid twisting
        glm::vec3 refUp(0.0f, 1.0f, 0.0f);

        for (int i = 0; i <= curveSegments; i++) {
            glm::vec3 T = tangents[i];
            glm::vec3 N, B;

            // Choose a reference that's not parallel to the tangent
            if (fabsf(glm::dot(T, refUp)) > 0.99f)
                refUp = glm::vec3(1.0f, 0.0f, 0.0f);

            N = glm::normalize(glm::cross(T, refUp));
            B = glm::normalize(glm::cross(N, T));
            // Update refUp to track smoothly
            refUp = B;

            float u = (float)i / (float)curveSegments;

            for (int j = 0; j <= crossSections; j++) {
                float angle = (float)j / (float)crossSections * 2.0f * glm::pi<float>();
                float cx = cosf(angle);
                float cy = sinf(angle);

                glm::vec3 offset = N * (cx * radius) + B * (cy * radius);
                glm::vec3 pos = points[i] + offset;
                glm::vec3 normal = glm::normalize(offset);

                float v = (float)j / (float)crossSections;

                outVertices.insert(outVertices.end(), {
                    pos.x, pos.y, pos.z,
                    normal.x, normal.y, normal.z,
                    u, v
                });
            }
        }

        // Generate indices
        for (int i = 0; i < curveSegments; i++) {
            for (int j = 0; j < crossSections; j++) {
                int curr = i * (crossSections + 1) + j;
                int next = curr + crossSections + 1;

                outIndices.push_back((unsigned)curr);
                outIndices.push_back((unsigned)next);
                outIndices.push_back((unsigned)(curr + 1));

                outIndices.push_back((unsigned)(curr + 1));
                outIndices.push_back((unsigned)next);
                outIndices.push_back((unsigned)(next + 1));
            }
        }
    }

    // Generate tube with end caps (useful for standalone tubes)
    static void generateWithCaps(
        const glm::vec3& P0, const glm::vec3& P1,
        const glm::vec3& P2, const glm::vec3& P3,
        float radius, int curveSegments, int crossSections,
        std::vector<float>& outVertices,
        std::vector<unsigned int>& outIndices)
    {
        generate(P0, P1, P2, P3, radius, curveSegments, crossSections, outVertices, outIndices);

        // Add start cap
        int baseIdx = (int)outVertices.size() / 8;
        glm::vec3 startCenter = P0;
        glm::vec3 startNormal = glm::normalize(P0 - P1);
        outVertices.insert(outVertices.end(), {
            startCenter.x, startCenter.y, startCenter.z,
            startNormal.x, startNormal.y, startNormal.z,
            0.5f, 0.5f
        });

        int centerStart = baseIdx;
        for (int j = 0; j <= crossSections; j++) {
            // First ring vertices are already at indices 0..crossSections
            int ringIdx = j;
            if (j < crossSections) {
                outIndices.push_back((unsigned)centerStart);
                outIndices.push_back((unsigned)(ringIdx + 1));
                outIndices.push_back((unsigned)ringIdx);
            }
        }

        // Add end cap
        baseIdx = (int)outVertices.size() / 8;
        glm::vec3 endCenter = P3;
        glm::vec3 endNormal = glm::normalize(P3 - P2);
        outVertices.insert(outVertices.end(), {
            endCenter.x, endCenter.y, endCenter.z,
            endNormal.x, endNormal.y, endNormal.z,
            0.5f, 0.5f
        });

        int centerEnd = baseIdx;
        int lastRingStart = curveSegments * (crossSections + 1);
        for (int j = 0; j < crossSections; j++) {
            int ringIdx = lastRingStart + j;
            outIndices.push_back((unsigned)centerEnd);
            outIndices.push_back((unsigned)ringIdx);
            outIndices.push_back((unsigned)(ringIdx + 1));
        }
    }
};

#endif
