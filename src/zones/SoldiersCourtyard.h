#ifndef SOLDIERSCOURTYARD_H
#define SOLDIERSCOURTYARD_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <cmath>
#include "Shader.h"
#include "primitives/Cube.h"
#include "primitives/Sphere.h"
#include "primitives/Plane.h"

// ================================================================
// SoldiersCourtyard:
// - Central open parade courtyard
// - Grid of stylized human soldiers built only from primitives
// ================================================================
class SoldiersCourtyard {
public:
    static constexpr float COURTYARD_CENTER_X = 8.0f;
    static constexpr float COURTYARD_CENTER_Z = 0.0f;
    static constexpr float COURTYARD_W = 48.0f;
    static constexpr float COURTYARD_D = 82.0f;

    void init() {}

    void update(float deltaTime) {
        if (paradeEnabled) {
            animTime += deltaTime * 3.2f;
        }
    }

    void toggleParade() { paradeEnabled = !paradeEnabled; }
    bool isParadeEnabled() const { return paradeEnabled; }

    void render(Shader& shader, const glm::mat4& I,
                Cube& cube, Sphere& sphere, Plane& plane) const
    {
        shader.setBool("useTexture", false);

        renderCourtyardPad(shader, I, plane);

        // 8 x 10 formation, centered in courtyard.
        const int rows = 8;
        const int cols = 10;
        const float spacingX = 4.4f;
        const float spacingZ = 6.0f;

        const float startX = COURTYARD_CENTER_X - (float)(cols - 1) * 0.5f * spacingX;
        const float startZ = COURTYARD_CENTER_Z - (float)(rows - 1) * 0.5f * spacingZ;

        int idx = 0;
        for (int r = 0; r < rows; r++) {
            for (int c = 0; c < cols; c++, idx++) {
                float sx = startX + c * spacingX;
                float sz = startZ + r * spacingZ;
                float phase = animTime + (float)idx * 0.42f;
                renderSoldier(shader, I, cube, sphere, glm::vec3(sx, 0.0f, sz), phase);
            }
        }

        shader.setBool("useTexture", false);
    }

private:
    bool paradeEnabled = false;
    float animTime = 0.0f;

    static inline const glm::vec3 COL_GROUND = glm::vec3(0.36f, 0.34f, 0.31f);
    static inline const glm::vec3 COL_UNIFORM = glm::vec3(0.22f, 0.27f, 0.20f);
    static inline const glm::vec3 COL_UNIFORM_DARK = glm::vec3(0.14f, 0.17f, 0.12f);
    static inline const glm::vec3 COL_BOOTS = glm::vec3(0.07f, 0.07f, 0.07f);
    static inline const glm::vec3 COL_SKIN = glm::vec3(0.62f, 0.51f, 0.42f);
    static inline const glm::vec3 COL_GUN_METAL = glm::vec3(0.12f, 0.12f, 0.12f);
    static inline const glm::vec3 COL_GUN_WOOD = glm::vec3(0.24f, 0.16f, 0.10f);

    static float radians(float deg) { return deg * glm::pi<float>() / 180.0f; }

    void applyMaterial(Shader& shader, const glm::vec3& color,
                       float ambientK, float diffuseK,
                       float specularK, float shininess) const
    {
        shader.setVec3("material.ambient", color * ambientK);
        shader.setVec3("material.diffuse", color * diffuseK);
        shader.setVec3("material.specular", glm::vec3(specularK));
        shader.setFloat("material.shininess", shininess);
        shader.setVec3("material.emissive", glm::vec3(0.0f));
    }

    void drawCube(Shader& shader, const Cube& cube,
                  const glm::mat4& model,
                  const glm::vec3& color,
                  float shininess) const
    {
        applyMaterial(shader, color, 0.26f, 0.92f, 0.12f, shininess);
        shader.setMat4("model", model);
        glBindVertexArray(cube.VAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
    }

    void drawSphere(Shader& shader, const Sphere& sphere,
                    const glm::mat4& model,
                    const glm::vec3& color,
                    float shininess) const
    {
        applyMaterial(shader, color, 0.28f, 0.90f, 0.20f, shininess);
        shader.setMat4("model", model);
        glBindVertexArray(sphere.VAO);
        glDrawElements(GL_TRIANGLES, sphere.indexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    void drawCubeCentered(Shader& shader, const Cube& cube,
                          const glm::mat4& base,
                          const glm::vec3& center,
                          const glm::vec3& size,
                          const glm::vec3& color,
                          float shininess) const
    {
        glm::mat4 model = glm::translate(base, center - size * 0.5f);
        model = model * glm::scale(glm::mat4(1.0f), size);
        drawCube(shader, cube, model, color, shininess);
    }

    void drawSphereCentered(Shader& shader, const Sphere& sphere,
                            const glm::mat4& base,
                            const glm::vec3& center,
                            float radius,
                            const glm::vec3& color,
                            float shininess) const
    {
        glm::mat4 model = glm::translate(base, center - glm::vec3(radius));
        model = model * glm::scale(glm::mat4(1.0f), glm::vec3(radius * 2.0f));
        drawSphere(shader, sphere, model, color, shininess);
    }

    void drawLimbSegmentYZ(Shader& shader, const Cube& cube,
                           const glm::mat4& base,
                           float xCenter,
                           float y0, float z0,
                           float y1, float z1,
                           float width, float depth,
                           const glm::vec3& color,
                           float shininess) const
    {
        const float dy = y1 - y0;
        const float dz = z1 - z0;
        const float len = sqrtf(dy * dy + dz * dz);
        if (len < 0.001f) return;

        const float ax = atan2f(dz, dy);
        glm::mat4 model = glm::translate(base, glm::vec3(xCenter - width * 0.5f, y0, z0 - depth * 0.5f));
        model = model * glm::rotate(glm::mat4(1.0f), ax, glm::vec3(1, 0, 0));
        model = model * glm::scale(glm::mat4(1.0f), glm::vec3(width, len, depth));
        drawCube(shader, cube, model, color, shininess);
    }

    void renderCourtyardPad(Shader& shader, const glm::mat4& I, Plane& plane) const {
        shader.setBool("useTexture", false);
        applyMaterial(shader, COL_GROUND, 0.25f, 0.95f, 0.05f, 3.0f);
        plane.draw(shader, I,
            COURTYARD_CENTER_X - COURTYARD_W * 0.5f, 0.028f,
            COURTYARD_CENTER_Z - COURTYARD_D * 0.5f,
            COURTYARD_W, 1.0f, COURTYARD_D,
            COL_GROUND, 3.0f);
    }

    void renderSoldier(Shader& shader, const glm::mat4& I,
                       Cube& cube, Sphere& sphere,
                       const glm::vec3& pos,
                       float phase) const
    {
        const float gait = paradeEnabled ? sinf(phase) : 0.0f;
        const float armSwing = paradeEnabled ? radians(38.0f) * gait : 0.0f;

        // Feet remain grounded: only stride along Z on the ground plane.
        const float leftFootZ = 0.06f + (paradeEnabled ? 0.17f * gait : 0.0f);
        const float rightFootZ = 0.06f - (paradeEnabled ? 0.17f * gait : 0.0f);
        const float leftKneePush = 0.10f + (paradeEnabled ? 0.10f * glm::max(0.0f, -gait) : 0.0f);
        const float rightKneePush = 0.10f + (paradeEnabled ? 0.10f * glm::max(0.0f, gait) : 0.0f);

        glm::mat4 base = glm::translate(I, pos);

        // Boots and feet (strictly ground-anchored)
        drawCubeCentered(shader, cube, base,
            glm::vec3(-0.12f, 0.04f, leftFootZ - 0.06f),
            glm::vec3(0.20f, 0.08f, 0.42f),
            COL_BOOTS, 8.0f);
        drawCubeCentered(shader, cube, base,
            glm::vec3(0.12f, 0.04f, rightFootZ - 0.06f),
            glm::vec3(0.20f, 0.08f, 0.42f),
            COL_BOOTS, 8.0f);

        // Pelvis, torso, neck, head, helmet brim
        drawCubeCentered(shader, cube, base,
            glm::vec3(0.0f, 0.90f, 0.0f),
            glm::vec3(0.44f, 0.20f, 0.28f),
            COL_UNIFORM_DARK, 10.0f);
        drawCubeCentered(shader, cube, base,
            glm::vec3(0.0f, 1.34f, 0.0f),
            glm::vec3(0.52f, 0.74f, 0.34f),
            COL_UNIFORM, 11.0f);
        drawCubeCentered(shader, cube, base,
            glm::vec3(0.0f, 1.74f, 0.0f),
            glm::vec3(0.16f, 0.10f, 0.14f),
            COL_SKIN, 10.0f);
        drawSphereCentered(shader, sphere, base,
            glm::vec3(0.0f, 1.96f, 0.0f),
            0.15f,
            COL_SKIN, 14.0f);
        drawCubeCentered(shader, cube, base,
            glm::vec3(0.0f, 2.08f, 0.0f),
            glm::vec3(0.48f, 0.08f, 0.34f),
            COL_UNIFORM_DARK, 10.0f);

        // Back rifle: parallel to torso and strapped close to the back.
        {
            // Torso depth is 0.34, so back face is near z=+0.17. Keep rifle just outside that plane.
            glm::mat4 rifle = glm::translate(base, glm::vec3(0.20f, 1.38f, 0.22f));
            rifle = rifle * glm::rotate(glm::mat4(1.0f), radians(-2.0f), glm::vec3(0, 0, 1));

            drawCubeCentered(shader, cube, rifle,
                glm::vec3(0.0f, 0.26f, 0.0f), glm::vec3(0.06f, 0.92f, 0.06f),
                COL_GUN_METAL, 20.0f);
            drawCubeCentered(shader, cube, rifle,
                glm::vec3(0.0f, 0.63f, -0.04f), glm::vec3(0.10f, 0.16f, 0.10f),
                COL_GUN_METAL, 14.0f);
            drawCubeCentered(shader, cube, rifle,
                glm::vec3(0.0f, -0.04f, 0.02f), glm::vec3(0.11f, 0.26f, 0.09f),
                COL_GUN_WOOD, 8.0f);
            drawCubeCentered(shader, cube, rifle,
                glm::vec3(0.0f, -0.16f, 0.08f), glm::vec3(0.14f, 0.10f, 0.12f),
                COL_GUN_WOOD, 8.0f);
            drawCubeCentered(shader, cube, rifle,
                glm::vec3(0.0f, 0.36f, 0.09f), glm::vec3(0.02f, 0.56f, 0.02f),
                glm::vec3(0.05f, 0.05f, 0.05f), 6.0f);
        }

        // Arms (pendulum swing)
        renderArm(shader, cube, base, glm::vec3(-0.31f, 1.62f, 0.0f), armSwing, true);
        renderArm(shader, cube, base, glm::vec3(0.31f, 1.62f, 0.0f), -armSwing, false);

        // Legs with planted feet and jointed segments
        renderLeg(shader, cube, base, -0.12f, leftFootZ, leftKneePush);
        renderLeg(shader, cube, base, 0.12f, rightFootZ, rightKneePush);
    }

    void renderArm(Shader& shader, Cube& cube, const glm::mat4& base,
                   const glm::vec3& shoulderOffset,
                   float swing,
                   bool left) const
    {
        glm::mat4 shoulder = glm::translate(base, shoulderOffset);
        shoulder = shoulder * glm::rotate(glm::mat4(1.0f), swing, glm::vec3(1, 0, 0));

        // Upper arm
        drawCubeCentered(shader, cube, shoulder,
            glm::vec3(0.0f, -0.20f, 0.0f),
            glm::vec3(0.14f, 0.40f, 0.13f),
            COL_UNIFORM, 10.0f);

        // Lower arm
        glm::mat4 elbow = shoulder * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.40f, 0.0f));
        elbow = elbow * glm::rotate(glm::mat4(1.0f), radians(14.0f + 7.0f * fabsf(swing)), glm::vec3(1, 0, 0));
        drawCubeCentered(shader, cube, elbow,
            glm::vec3(0.0f, -0.18f, 0.0f),
            glm::vec3(0.12f, 0.34f, 0.12f),
            COL_UNIFORM_DARK, 9.0f);

        // Hand
        drawCubeCentered(shader, cube, elbow,
            glm::vec3(0.0f, -0.39f, 0.0f),
            glm::vec3(0.10f, 0.10f, 0.10f),
            COL_SKIN, 8.0f);

        // Shoulder cap for cleaner alignment to torso
        drawCubeCentered(shader, cube, base,
            shoulderOffset + glm::vec3(left ? -0.03f : 0.03f, 0.0f, 0.0f),
            glm::vec3(0.11f, 0.11f, 0.11f),
            COL_UNIFORM_DARK, 8.0f);
    }

    void renderLeg(Shader& shader, Cube& cube, const glm::mat4& base,
                   float xCenter,
                   float footZ,
                   float kneeForward) const
    {
        const float hipY = 0.88f;
        const float hipZ = 0.00f;
        const float ankleY = 0.10f;
        const float ankleZ = footZ;
        const float kneeY = 0.50f + 0.05f * fabsf(footZ - hipZ);
        const float kneeZ = (hipZ + ankleZ) * 0.5f + kneeForward;

        drawLimbSegmentYZ(shader, cube, base,
            xCenter, hipY, hipZ, kneeY, kneeZ,
            0.14f, 0.12f, COL_UNIFORM_DARK, 10.0f);
        drawLimbSegmentYZ(shader, cube, base,
            xCenter, kneeY, kneeZ, ankleY, ankleZ,
            0.13f, 0.11f, COL_UNIFORM_DARK, 10.0f);
    }
};

#endif
