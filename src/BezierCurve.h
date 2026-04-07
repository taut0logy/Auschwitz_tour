#ifndef BEZIERCURVE_H
#define BEZIERCURVE_H

#include <glm/glm.hpp>
#include <vector>
#include <cmath>

class BezierCurve {
public:
    // Evaluate cubic Bezier at parameter t ∈ [0,1]
    static glm::vec3 evaluateCubic(glm::vec3 P0, glm::vec3 P1, glm::vec3 P2, glm::vec3 P3, float t) {
        float u = 1.0f - t;
        return u*u*u*P0 + 3.0f*u*u*t*P1 + 3.0f*u*t*t*P2 + t*t*t*P3;
    }

    // Evaluate Catmull-Rom spline at parameter t ∈ [0,1]
    static glm::vec3 evaluateCatmullRom(glm::vec3 P0, glm::vec3 P1, glm::vec3 P2, glm::vec3 P3, float t) {
        float t2 = t * t;
        float t3 = t2 * t;
        return 0.5f * ((2.0f*P1) +
            (-P0 + P2) * t +
            (2.0f*P0 - 5.0f*P1 + 4.0f*P2 - P3) * t2 +
            (-P0 + 3.0f*P1 - 3.0f*P2 + P3) * t3);
    }

    // Generate points along a cubic Bezier curve
    static std::vector<glm::vec3> sampleCubic(glm::vec3 P0, glm::vec3 P1, glm::vec3 P2, glm::vec3 P3, int segments) {
        std::vector<glm::vec3> points;
        for (int i = 0; i <= segments; i++) {
            float t = (float)i / (float)segments;
            points.push_back(evaluateCubic(P0, P1, P2, P3, t));
        }
        return points;
    }

    // Generate an arch mesh by extruding a rectangle along a Bezier curve
    // Returns vertex data: pos(3) + normal(3) + uv(2)
    static void generateArchMesh(glm::vec3 P0, glm::vec3 P1, glm::vec3 P2, glm::vec3 P3,
                                  int segments, float width, float depth,
                                  std::vector<float>& vertices, std::vector<unsigned int>& indices)
    {
        auto points = sampleCubic(P0, P1, P2, P3, segments);

        // For each point on the curve, create a rectangular cross-section
        for (int i = 0; i <= segments; i++) {
            glm::vec3 p = points[i];
            float u = (float)i / (float)segments;

            // Tangent direction
            glm::vec3 tangent;
            if (i < segments)
                tangent = glm::normalize(points[i + 1] - points[i]);
            else
                tangent = glm::normalize(points[i] - points[i - 1]);

            // Cross-section normal (perpendicular to tangent in XY plane)
            glm::vec3 up(0, 0, 1);
            glm::vec3 right = glm::normalize(glm::cross(tangent, up));
            glm::vec3 normal = glm::normalize(glm::cross(right, tangent));

            float hw = width * 0.5f;
            float hd = depth * 0.5f;

            // 4 corners of cross-section at this point
            // Top
            vertices.insert(vertices.end(), {
                p.x + normal.x*hw, p.y + normal.y*hw, p.z + hd,
                normal.x, normal.y, 0.0f,
                u, 1.0f
            });
            // Bottom
            vertices.insert(vertices.end(), {
                p.x - normal.x*hw, p.y - normal.y*hw, p.z + hd,
                -normal.x, -normal.y, 0.0f,
                u, 0.0f
            });
            // Top back
            vertices.insert(vertices.end(), {
                p.x + normal.x*hw, p.y + normal.y*hw, p.z - hd,
                normal.x, normal.y, 0.0f,
                u, 1.0f
            });
            // Bottom back
            vertices.insert(vertices.end(), {
                p.x - normal.x*hw, p.y - normal.y*hw, p.z - hd,
                -normal.x, -normal.y, 0.0f,
                u, 0.0f
            });
        }

        // Generate indices connecting adjacent cross-sections
        for (int i = 0; i < segments; i++) {
            int base = i * 4;
            int next = (i + 1) * 4;

            // Front face (top-bottom of adjacent sections)
            indices.insert(indices.end(), {
                (unsigned)(base+0), (unsigned)(next+0), (unsigned)(base+1),
                (unsigned)(base+1), (unsigned)(next+0), (unsigned)(next+1)
            });
            // Back face
            indices.insert(indices.end(), {
                (unsigned)(base+2), (unsigned)(base+3), (unsigned)(next+2),
                (unsigned)(base+3), (unsigned)(next+3), (unsigned)(next+2)
            });
            // Top face
            indices.insert(indices.end(), {
                (unsigned)(base+0), (unsigned)(base+2), (unsigned)(next+0),
                (unsigned)(base+2), (unsigned)(next+2), (unsigned)(next+0)
            });
            // Bottom face
            indices.insert(indices.end(), {
                (unsigned)(base+1), (unsigned)(next+1), (unsigned)(base+3),
                (unsigned)(base+3), (unsigned)(next+1), (unsigned)(next+3)
            });
        }
    }

    // Generate ruled surface between two Bezier curves (for crematorium roof)
    static void generateRuledSurface(glm::vec3 curveA[4], glm::vec3 curveB[4],
                                      int uSegments, int vSegments,
                                      std::vector<float>& vertices, std::vector<unsigned int>& indices)
    {
        for (int j = 0; j <= vSegments; j++) {
            float v = (float)j / (float)vSegments;
            for (int i = 0; i <= uSegments; i++) {
                float u = (float)i / (float)uSegments;
                glm::vec3 pA = evaluateCubic(curveA[0], curveA[1], curveA[2], curveA[3], u);
                glm::vec3 pB = evaluateCubic(curveB[0], curveB[1], curveB[2], curveB[3], u);
                glm::vec3 p = pA + v * (pB - pA);

                // Approximate normal via cross product of partial derivatives
                glm::vec3 du, dv;
                float eps = 0.01f;
                float u2 = std::min(u + eps, 1.0f);
                glm::vec3 pA2 = evaluateCubic(curveA[0], curveA[1], curveA[2], curveA[3], u2);
                glm::vec3 pB2 = evaluateCubic(curveB[0], curveB[1], curveB[2], curveB[3], u2);
                du = (pA2 + v*(pB2-pA2)) - p;
                dv = pB - pA;
                glm::vec3 normal = glm::normalize(glm::cross(du, dv));
                if (normal.y < 0) normal = -normal; // Ensure upward

                vertices.insert(vertices.end(), {p.x, p.y, p.z, normal.x, normal.y, normal.z, u, v});
            }
        }

        for (int j = 0; j < vSegments; j++) {
            for (int i = 0; i < uSegments; i++) {
                int a = j * (uSegments + 1) + i;
                int b = a + 1;
                int c = a + uSegments + 1;
                int d = c + 1;
                indices.insert(indices.end(), {(unsigned)a, (unsigned)c, (unsigned)b,
                                               (unsigned)b, (unsigned)c, (unsigned)d});
            }
        }
    }
};

#endif
