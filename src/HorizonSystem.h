#ifndef HORIZONSYSTEM_H
#define HORIZONSYSTEM_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <vector>
#include <cmath>
#include "Shader.h"
#include "primitives/Plane.h"
#include "primitives/Cube.h"

// ================================================================
// HorizonSystem: 3-layer parallax
// Layer 1: Ground extension plane (200m beyond fence)
// Layer 2: Mid-ground billboard treeline ring (50m outside fence)
// Layer 3: Far horizon cylindrical billboard (r=280m)
// Layer 4: Ground fog quad
// ================================================================
class HorizonSystem {
public:
    GLuint horizCylVAO = 0, horizCylVBO = 0;
    int horizCylVertCount = 0;

    GLuint treelineVAO = 0, treelineVBO = 0;
    int treelineVertCount = 0;

    void init(unsigned int texHorizon) {
        buildHorizonCylinder();
        buildTreelineRing();
    }

    // Render opaque layer (ground extension)
    void renderGroundExtension(Shader& shader, const glm::mat4& I,
                               Plane& plane, unsigned int texGravel) const
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texGravel);
        shader.setInt("texture1", 0);
        shader.setBool("useTexture", texGravel != 0);
        shader.setFloat("texRepeat", 80.0f);
        shader.setVec3("material.ambient", glm::vec3(0.15f));
        shader.setVec3("material.diffuse", glm::vec3(0.35f, 0.38f, 0.30f));
        shader.setVec3("material.specular", glm::vec3(0.01f));
        shader.setFloat("material.shininess", 2.0f);
        shader.setVec3("material.emissive", glm::vec3(0.0f));

        // Large plane surrounding the camp (beyond fence)
        plane.draw(shader, I, -340.0f, -0.1f, -300.0f, 680.0f, 1.0f, 600.0f,
                   glm::vec3(0.30f, 0.32f, 0.25f), 2.0f);
        shader.setBool("useTexture", false);
    }

    // Render alpha layers (treeline + horizon cylinder + fog)
    void renderAlphaLayers(Shader& alphaShader, const glm::mat4& I,
                           unsigned int texHorizon, glm::vec3 fogColor) const
    {
        // Layer 2: Billboard treeline ring
        if (treelineVertCount > 0) {
            alphaShader.setBool("useTexture", texHorizon != 0);
            if (texHorizon) {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, texHorizon);
                alphaShader.setInt("texture1", 0);
                alphaShader.setFloat("texRepeat", 1.0f);
            }
            alphaShader.setVec3("material.ambient", glm::vec3(0.1f));
            alphaShader.setVec3("material.diffuse", glm::vec3(0.2f, 0.25f, 0.15f));
            alphaShader.setVec3("material.specular", glm::vec3(0.0f));
            alphaShader.setVec3("material.emissive", glm::vec3(0.0f));
            alphaShader.setFloat("material.shininess", 1.0f);
            alphaShader.setMat4("model", I);
            glBindVertexArray(treelineVAO);
            glDrawArrays(GL_TRIANGLES, 0, treelineVertCount);
            glBindVertexArray(0);
        }

        // Layer 3: Horizon cylinder
        if (horizCylVertCount > 0) {
            alphaShader.setBool("useTexture", texHorizon != 0);
            if (texHorizon) {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, texHorizon);
                alphaShader.setInt("texture1", 0);
                alphaShader.setFloat("texRepeat", 1.0f);
            }
            alphaShader.setVec3("material.ambient", glm::vec3(0.5f));
            alphaShader.setVec3("material.diffuse", glm::vec3(0.6f));
            alphaShader.setVec3("material.specular", glm::vec3(0.0f));
            alphaShader.setVec3("material.emissive", fogColor * 0.3f);
            alphaShader.setMat4("model", I);
            glBindVertexArray(horizCylVAO);
            glDrawArrays(GL_TRIANGLES, 0, horizCylVertCount);
            glBindVertexArray(0);
        }
    }

    // Ground fog quad
    void renderGroundFog(Shader& alphaShader, const glm::mat4& I,
                         Cube& cube, float nightFactor) const
    {
        float fogAlpha = 0.15f * nightFactor; // Only at night
        if (fogAlpha < 0.01f) return;

        alphaShader.setBool("useTexture", false);
        alphaShader.setFloat("objectAlpha", fogAlpha);
        alphaShader.setVec3("material.ambient", glm::vec3(0.06f, 0.06f, 0.07f));
        alphaShader.setVec3("material.diffuse", glm::vec3(0.18f, 0.18f, 0.20f));
        alphaShader.setVec3("material.specular", glm::vec3(0.0f));
        alphaShader.setVec3("material.emissive", glm::vec3(0.0f));
        alphaShader.setFloat("material.shininess", 1.0f);

        cube.draw(alphaShader, I, -200.0f, 0.3f, -150.0f, 400.0f, 0.02f, 300.0f,
                  glm::vec3(0.7f, 0.7f, 0.75f), 1.0f);
        alphaShader.setFloat("objectAlpha", 1.0f);
    }

    void cleanup() {
        if (horizCylVAO) { glDeleteVertexArrays(1, &horizCylVAO); glDeleteBuffers(1, &horizCylVBO); }
        if (treelineVAO) { glDeleteVertexArrays(1, &treelineVAO); glDeleteBuffers(1, &treelineVBO); }
    }

private:
    void buildHorizonCylinder() {
        // Cylindrical backdrop, r = 280m, h = 60m, 128 segments, inner face
        int segments = 128;
        float r = 280.0f, h = 60.0f;
        std::vector<float> verts;

        for (int i = 0; i < segments; i++) {
            float a1 = (float)i / segments * 2.0f * glm::pi<float>();
            float a2 = (float)(i + 1) / segments * 2.0f * glm::pi<float>();

            float x1 = cosf(a1) * r, z1 = sinf(a1) * r;
            float x2 = cosf(a2) * r, z2 = sinf(a2) * r;
            float u1 = (float)i / segments, u2 = (float)(i + 1) / segments;

            // Normal faces inward (negative radial)
            float nx1 = -cosf(a1), nz1 = -sinf(a1);
            float nx2 = -cosf(a2), nz2 = -sinf(a2);

            // Two triangles for inner face
            // Bottom-left, top-right, top-left
            verts.insert(verts.end(), { x1, -10.0f, z1, nx1, 0, nz1, u1, 0 });
            verts.insert(verts.end(), { x2, h - 10.0f, z2, nx2, 0, nz2, u2, 1 });
            verts.insert(verts.end(), { x1, h - 10.0f, z1, nx1, 0, nz1, u1, 1 });
            // Bottom-left, bottom-right, top-right
            verts.insert(verts.end(), { x1, -10.0f, z1, nx1, 0, nz1, u1, 0 });
            verts.insert(verts.end(), { x2, -10.0f, z2, nx2, 0, nz2, u2, 0 });
            verts.insert(verts.end(), { x2, h - 10.0f, z2, nx2, 0, nz2, u2, 1 });
        }

        horizCylVertCount = (int)verts.size() / 8;

        glGenVertexArrays(1, &horizCylVAO);
        glGenBuffers(1, &horizCylVBO);
        glBindVertexArray(horizCylVAO);
        glBindBuffer(GL_ARRAY_BUFFER, horizCylVBO);
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

    void buildTreelineRing() {
        // Billboard trees placed 50m outside fence in a ring
        // Crossed-plane pairs at 5m intervals, 8-14m tall
        std::vector<float> verts;
        float fenceX = 135.0f, fenceZ = 95.0f;
        float treeOffset = 50.0f;

        auto addTree = [&](float x, float z, float height) {
            float hw = 2.5f;  // half width
            glm::vec3 n(0, 0, 1);
            // Plane 1 (N-S aligned)
            verts.insert(verts.end(), { x - hw, 0, z, n.x, n.y, n.z, 0, 0 });
            verts.insert(verts.end(), { x + hw, 0, z, n.x, n.y, n.z, 1, 0 });
            verts.insert(verts.end(), { x + hw, height, z, n.x, n.y, n.z, 1, 1 });
            verts.insert(verts.end(), { x + hw, height, z, n.x, n.y, n.z, 1, 1 });
            verts.insert(verts.end(), { x - hw, height, z, n.x, n.y, n.z, 0, 1 });
            verts.insert(verts.end(), { x - hw, 0, z, n.x, n.y, n.z, 0, 0 });
            // Plane 2 (E-W aligned, crossed)
            glm::vec3 n2(1, 0, 0);
            verts.insert(verts.end(), { x, 0, z - hw, n2.x, n2.y, n2.z, 0, 0 });
            verts.insert(verts.end(), { x, 0, z + hw, n2.x, n2.y, n2.z, 1, 0 });
            verts.insert(verts.end(), { x, height, z + hw, n2.x, n2.y, n2.z, 1, 1 });
            verts.insert(verts.end(), { x, height, z + hw, n2.x, n2.y, n2.z, 1, 1 });
            verts.insert(verts.end(), { x, height, z - hw, n2.x, n2.y, n2.z, 0, 1 });
            verts.insert(verts.end(), { x, 0, z - hw, n2.x, n2.y, n2.z, 0, 0 });
        };

        srand(123);
        // North side
        for (float x = -fenceX - treeOffset; x <= fenceX + treeOffset; x += 5.0f) {
            float h = 8.0f + (rand() % 7);
            addTree(x, -fenceZ - treeOffset, h);
        }
        // South side
        for (float x = -fenceX - treeOffset; x <= fenceX + treeOffset; x += 5.0f) {
            float h = 8.0f + (rand() % 7);
            addTree(x, fenceZ + treeOffset, h);
        }
        // West side
        for (float z = -fenceZ - treeOffset; z <= fenceZ + treeOffset; z += 5.0f) {
            float h = 8.0f + (rand() % 7);
            addTree(-fenceX - treeOffset, z, h);
        }
        // East side
        for (float z = -fenceZ - treeOffset; z <= fenceZ + treeOffset; z += 5.0f) {
            float h = 8.0f + (rand() % 7);
            addTree(fenceX + treeOffset, z, h);
        }

        treelineVertCount = (int)verts.size() / 8;
        if (treelineVertCount == 0) return;

        glGenVertexArrays(1, &treelineVAO);
        glGenBuffers(1, &treelineVBO);
        glBindVertexArray(treelineVAO);
        glBindBuffer(GL_ARRAY_BUFFER, treelineVBO);
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
