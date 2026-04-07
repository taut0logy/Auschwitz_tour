#ifndef FENCESYSTEM_H
#define FENCESYSTEM_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <cmath>
#include "Shader.h"
#include "BezierCurve.h"
#include "primitives/Cylinder.h"

// ================================================================
// FenceSystem: Dual perimeter fence (inner + outer), concrete posts,
// 5-strand barbed wire with Bezier droop
// Per Sections 2.4, 7.3
// ================================================================
class FenceSystem {
public:
    GLuint wireVAO = 0, wireVBO = 0;
    int wireVertexCount = 0;

    // Inner fence: X = ±133, Z = ±93
    // Outer fence: X = ±135, Z = ±95
    static constexpr float INNER_X = 133.0f, INNER_Z = 93.0f;
    static constexpr float OUTER_X = 135.0f, OUTER_Z = 95.0f;
    static constexpr float POST_SPACING = 3.5f;
    static constexpr float POST_RADIUS = 0.06f;  // 0.12m diameter
    static constexpr float POST_HEIGHT = 2.8f;

    void init() {
        buildFenceWire();
    }

    void renderPosts(Shader& shader, const glm::mat4& I,
                     Cylinder& cyl, unsigned int texConcrete) const
    {
        bindTex(shader, texConcrete, 2.0f);
        setMaterial(shader, COL_CONCRETE, 0.18f, 0.82f, 0.06f, 6.0f);

        // Inner fence posts
        renderFenceRun(shader, I, cyl, -INNER_X, -INNER_Z, INNER_X, -INNER_Z); // North
        renderFenceRun(shader, I, cyl, -INNER_X, INNER_Z, INNER_X, INNER_Z);   // South
        renderFenceRun(shader, I, cyl, -INNER_X, -INNER_Z, -INNER_X, INNER_Z); // West
        
        // East fence spans across X = INNER_X, split for gate at Z=0
        renderFenceRun(shader, I, cyl, INNER_X, -INNER_Z, INNER_X, -10.0f);    // East (North half)
        renderFenceRun(shader, I, cyl, INNER_X, 10.0f, INNER_X, INNER_Z);      // East (South half)

        // Outer fence posts (offset 2m outward)
        renderFenceRun(shader, I, cyl, -OUTER_X, -OUTER_Z, OUTER_X, -OUTER_Z); // North
        renderFenceRun(shader, I, cyl, -OUTER_X, OUTER_Z, OUTER_X, OUTER_Z);   // South
        renderFenceRun(shader, I, cyl, -OUTER_X, -OUTER_Z, -OUTER_X, OUTER_Z); // West
        
        // East fence for outer fence split as well
        renderFenceRun(shader, I, cyl, OUTER_X, -OUTER_Z, OUTER_X, -10.0f);    // East (North half)
        renderFenceRun(shader, I, cyl, OUTER_X, 10.0f, OUTER_X, OUTER_Z);      // East (South half)

        unbind(shader);
    }

    void renderWire(Shader& shader, const glm::mat4& I) const {
        if (wireVertexCount <= 0) return;
        shader.setMat4("model", I);
        glBindVertexArray(wireVAO);
        glDrawArrays(GL_LINES, 0, wireVertexCount);
        glBindVertexArray(0);
    }

    void cleanup() {
        if (wireVAO) { glDeleteVertexArrays(1, &wireVAO); glDeleteBuffers(1, &wireVBO); }
    }

private:
    static inline const glm::vec3 COL_CONCRETE = glm::vec3(0.60f, 0.58f, 0.55f);
    static inline const glm::vec3 COL_METAL    = glm::vec3(0.50f, 0.48f, 0.46f);

    // Wire heights per spec: Y = 0.5, 0.9, 1.4, 1.9, 2.5 m
    static constexpr float WIRE_HEIGHTS[5] = { 0.5f, 0.9f, 1.4f, 1.9f, 2.5f };
    static constexpr float WIRE_DROOP = 0.10f;

    void bindTex(Shader& s, unsigned int tex, float rep) const {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex);
        s.setInt("texture1", 0);
        s.setBool("useTexture", tex != 0);
        s.setFloat("texRepeat", rep);
    }
    void unbind(Shader& s) const { s.setBool("useTexture", false); }
    void setMaterial(Shader& s, glm::vec3 col, float a, float d, float sp, float sh) const {
        s.setVec3("material.ambient", col * a);
        s.setVec3("material.diffuse", col * d);
        s.setVec3("material.specular", glm::vec3(sp));
        s.setFloat("material.shininess", sh);
        s.setVec3("material.emissive", glm::vec3(0.0f));
    }

    void renderFenceRun(Shader& shader, const glm::mat4& I,
                        Cylinder& cyl, float x1, float z1, float x2, float z2) const
    {
        float dx = x2 - x1, dz = z2 - z1;
        float dist = sqrtf(dx * dx + dz * dz);
        int numPosts = (int)(dist / POST_SPACING) + 1;
        float dirX = dx / dist, dirZ = dz / dist;

        for (int p = 0; p < numPosts; p++) {
            float px = x1 + p * POST_SPACING * dirX;
            float pz = z1 + p * POST_SPACING * dirZ;
            cyl.draw(shader, I, px - POST_RADIUS, 0.0f, pz - POST_RADIUS,
                     POST_RADIUS * 2, POST_HEIGHT, POST_RADIUS * 2, COL_CONCRETE, 8.0f);
        }
    }

    void buildFenceWire() {
        std::vector<float> verts;
        int segments = 16;

        auto addWireRun = [&](float x1, float z1, float x2, float z2) {
            float dx = x2 - x1, dz = z2 - z1;
            float dist = sqrtf(dx * dx + dz * dz);
            int numBays = (int)(dist / POST_SPACING);
            if (numBays < 1) return;
            float dirX = dx / dist, dirZ = dz / dist;

            for (int wire = 0; wire < 5; wire++) {
                float wireY = WIRE_HEIGHTS[wire];
                for (int p = 0; p < numBays; p++) {
                    float ax = x1 + p * POST_SPACING * dirX;
                    float az = z1 + p * POST_SPACING * dirZ;
                    float bx = x1 + (p + 1) * POST_SPACING * dirX;
                    float bz = z1 + (p + 1) * POST_SPACING * dirZ;

                    glm::vec3 P0(ax, wireY, az);
                    glm::vec3 P3(bx, wireY, bz);
                    glm::vec3 P1 = P0 + glm::vec3((bx - ax) * 0.33f, -WIRE_DROOP, (bz - az) * 0.33f);
                    glm::vec3 P2 = P3 + glm::vec3((ax - bx) * 0.33f, -WIRE_DROOP, (az - bz) * 0.33f);

                    for (int s = 0; s < segments; s++) {
                        float t0 = (float)s / segments;
                        float t1 = (float)(s + 1) / segments;
                        glm::vec3 a = BezierCurve::evaluateCubic(P0, P1, P2, P3, t0);
                        glm::vec3 b = BezierCurve::evaluateCubic(P0, P1, P2, P3, t1);
                        // pos(3) + normal(3) + uv(2)
                        verts.insert(verts.end(), { a.x, a.y, a.z, 0, 1, 0, t0, 0 });
                        verts.insert(verts.end(), { b.x, b.y, b.z, 0, 1, 0, t1, 0 });
                    }
                }
            }
        };

        // Inner fence
        addWireRun(-INNER_X, -INNER_Z, INNER_X, -INNER_Z);
        addWireRun(-INNER_X, INNER_Z, INNER_X, INNER_Z);
        addWireRun(-INNER_X, -INNER_Z, -INNER_X, INNER_Z);
        addWireRun(INNER_X, -INNER_Z, INNER_X, -10.0f);
        addWireRun(INNER_X, 10.0f, INNER_X, INNER_Z);

        // Outer fence
        addWireRun(-OUTER_X, -OUTER_Z, OUTER_X, -OUTER_Z);
        addWireRun(-OUTER_X, OUTER_Z, OUTER_X, OUTER_Z);
        addWireRun(-OUTER_X, -OUTER_Z, -OUTER_X, OUTER_Z);
        addWireRun(OUTER_X, -OUTER_Z, OUTER_X, -10.0f);
        addWireRun(OUTER_X, 10.0f, OUTER_X, OUTER_Z);

        wireVertexCount = (int)verts.size() / 8;
        if (wireVertexCount == 0) return;

        glGenVertexArrays(1, &wireVAO);
        glGenBuffers(1, &wireVBO);
        glBindVertexArray(wireVAO);
        glBindBuffer(GL_ARRAY_BUFFER, wireVBO);
        glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);

        int stride = 8 * sizeof(float);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glBindVertexArray(0);
    }
};

#endif
