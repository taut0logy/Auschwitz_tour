#ifndef LIGHTCULLER_H
#define LIGHTCULLER_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <algorithm>
#include <cmath>

// ================================================================
// LightCuller: Frustum-based light culling for street lamp spotlights
// Per Section 4.2
// Caps GPU lighting at 24 nearest lamps + 12 tower spots = 36 max
// ================================================================
class LightCuller {
public:
    struct FrustumPlane {
        glm::vec3 normal;
        float distance;

        float distToPoint(const glm::vec3& p) const {
            return glm::dot(normal, p) + distance;
        }
    };

    FrustumPlane planes[6]; // Left, Right, Bottom, Top, Near, Far

    void extractFrustum(const glm::mat4& viewProj) {
        // Extract frustum planes from combined view-projection matrix
        // Left
        planes[0].normal.x = viewProj[0][3] + viewProj[0][0];
        planes[0].normal.y = viewProj[1][3] + viewProj[1][0];
        planes[0].normal.z = viewProj[2][3] + viewProj[2][0];
        planes[0].distance = viewProj[3][3] + viewProj[3][0];
        // Right
        planes[1].normal.x = viewProj[0][3] - viewProj[0][0];
        planes[1].normal.y = viewProj[1][3] - viewProj[1][0];
        planes[1].normal.z = viewProj[2][3] - viewProj[2][0];
        planes[1].distance = viewProj[3][3] - viewProj[3][0];
        // Bottom
        planes[2].normal.x = viewProj[0][3] + viewProj[0][1];
        planes[2].normal.y = viewProj[1][3] + viewProj[1][1];
        planes[2].normal.z = viewProj[2][3] + viewProj[2][1];
        planes[2].distance = viewProj[3][3] + viewProj[3][1];
        // Top
        planes[3].normal.x = viewProj[0][3] - viewProj[0][1];
        planes[3].normal.y = viewProj[1][3] - viewProj[1][1];
        planes[3].normal.z = viewProj[2][3] - viewProj[2][1];
        planes[3].distance = viewProj[3][3] - viewProj[3][1];
        // Near
        planes[4].normal.x = viewProj[0][3] + viewProj[0][2];
        planes[4].normal.y = viewProj[1][3] + viewProj[1][2];
        planes[4].normal.z = viewProj[2][3] + viewProj[2][2];
        planes[4].distance = viewProj[3][3] + viewProj[3][2];
        // Far
        planes[5].normal.x = viewProj[0][3] - viewProj[0][2];
        planes[5].normal.y = viewProj[1][3] - viewProj[1][2];
        planes[5].normal.z = viewProj[2][3] - viewProj[2][2];
        planes[5].distance = viewProj[3][3] - viewProj[3][2];

        // Normalize planes
        for (int i = 0; i < 6; i++) {
            float len = glm::length(planes[i].normal);
            if (len > 0.0001f) {
                planes[i].normal /= len;
                planes[i].distance /= len;
            }
        }
    }

    // Test if a point is within the frustum (with some radius buffer)
    bool isInFrustum(const glm::vec3& center, float radius = 30.0f) const {
        for (int i = 0; i < 6; i++) {
            if (planes[i].distToPoint(center) < -radius)
                return false;
        }
        return true;
    }

    // Cull lamp spotlights: return sorted indices of the nearest N visible lamps
    // cameraPos: camera world position
    // positions: array of lamp positions
    // count: total number of lamps
    // maxResults: maximum number of spotlight slots (24 for lamps)
    std::vector<int> cullLamps(const glm::vec3& cameraPos,
                               const glm::vec3* positions, int count,
                               int maxResults = 24) const
    {
        struct CullEntry {
            int index;
            float distSq;
        };

        std::vector<CullEntry> visible;
        visible.reserve(count);

        for (int i = 0; i < count; i++) {
            if (isInFrustum(positions[i], 30.0f)) {
                float dx = positions[i].x - cameraPos.x;
                float dz = positions[i].z - cameraPos.z;
                visible.push_back({ i, dx * dx + dz * dz });
            }
        }

        // Sort by distance (nearest first)
        std::sort(visible.begin(), visible.end(),
            [](const CullEntry& a, const CullEntry& b) { return a.distSq < b.distSq; });

        std::vector<int> result;
        int n = std::min((int)visible.size(), maxResults);
        result.reserve(n);
        for (int i = 0; i < n; i++)
            result.push_back(visible[i].index);

        return result;
    }
};

#endif
