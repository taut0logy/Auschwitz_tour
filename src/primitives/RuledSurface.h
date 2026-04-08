#ifndef RULEDSURFACE_H
#define RULEDSURFACE_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <cmath>
#include <algorithm>
#include "BezierCurve.h"

class RuledSurface {
public:
    static void generate(
        const glm::vec3 curveA[4], const glm::vec3 curveB[4],
        int uSegments, int vSegments,
        std::vector<float>& outVertices,
        std::vector<unsigned int>& outIndices)
    {
        outVertices.clear();
        outIndices.clear();

        for (int j = 0; j <= vSegments; j++) {
            float v = (float)j / (float)vSegments;
            for (int i = 0; i <= uSegments; i++) {
                float u = (float)i / (float)uSegments;

                glm::vec3 pA = BezierCurve::evaluateCubic(curveA[0], curveA[1], curveA[2], curveA[3], u);
                glm::vec3 pB = BezierCurve::evaluateCubic(curveB[0], curveB[1], curveB[2], curveB[3], u);
                glm::vec3 p = pA + v * (pB - pA);

                // Approximate normal via cross product of partial derivatives
                float eps = 0.01f;
                float u2 = std::min(u + eps, 1.0f);
                glm::vec3 pA2 = BezierCurve::evaluateCubic(curveA[0], curveA[1], curveA[2], curveA[3], u2);
                glm::vec3 pB2 = BezierCurve::evaluateCubic(curveB[0], curveB[1], curveB[2], curveB[3], u2);
                glm::vec3 du = (pA2 + v * (pB2 - pA2)) - p;
                glm::vec3 dv = pB - pA;

                glm::vec3 normal = glm::normalize(glm::cross(du, dv));
                if (normal.y < 0) normal = -normal; // Ensure upward-facing

                outVertices.insert(outVertices.end(), {
                    p.x, p.y, p.z,
                    normal.x, normal.y, normal.z,
                    u, v
                });
            }
        }

        for (int j = 0; j < vSegments; j++) {
            for (int i = 0; i < uSegments; i++) {
                int a = j * (uSegments + 1) + i;
                int b = a + 1;
                int c = a + uSegments + 1;
                int d = c + 1;
                outIndices.insert(outIndices.end(), {
                    (unsigned)a, (unsigned)c, (unsigned)b,
                    (unsigned)b, (unsigned)c, (unsigned)d
                });
            }
        }
    }
};

#endif
