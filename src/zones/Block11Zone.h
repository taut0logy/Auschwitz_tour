#ifndef BLOCK11ZONE_H
#define BLOCK11ZONE_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Shader.h"
#include "primitives/Cube.h"
#include "primitives/Plane.h"

// ================================================================
// Block11Zone: Courtyard enclosure, Death Wall, Block 11 courtyard
// ================================================================
class Block11Zone {
public:
    void render(Shader& shader, const glm::mat4& I,
                Cube& cube, Plane& plane,
                unsigned int texBrickDark, unsigned int texWoodDark,
                unsigned int texGravel) const
    {
        float cwX1 = -55.0f, cwX2 = -41.0f; // courtyard X bounds (matching gap)
        float cwZ1 = -34.0f, cwZ2 = -22.0f; // courtyard Z bounds
        float wallH = 3.5f, wallT = 0.3f;

        // ---- High enclosure walls ----
        bindTex(shader, texBrickDark, 6.0f);
        setMaterial(shader, COL_BRICK_DARK, 0.06f, 0.65f, 0.03f, 4.0f);

        // West wall (between blocks)
        cube.draw(shader, I, cwX1, 0.0f, cwZ1, wallT, wallH, cwZ2 - cwZ1, COL_BRICK_DARK, 4.0f);
        // East wall
        cube.draw(shader, I, cwX2 - wallT, 0.0f, cwZ1, wallT, wallH, cwZ2 - cwZ1, COL_BRICK_DARK, 4.0f);
        // South wall
        cube.draw(shader, I, cwX1, 0.0f, cwZ1, cwX2 - cwX1, wallH, wallT, COL_BRICK_DARK, 4.0f);
        unbind(shader);

        // ---- Death Wall ----
        // Wooden shooting wall, 5m wide × 3m tall
        // At Z = -22 (north end of courtyard), centred in courtyard
        float deathWallCX = (cwX1 + cwX2) * 0.5f;
        bindTex(shader, texWoodDark, 4.0f);
        setMaterial(shader, COL_WOOD_DARK, 0.15f, 0.60f, 0.08f, 8.0f);
        cube.draw(shader, I, deathWallCX - 2.5f, 0.0f, cwZ2 - wallT, 5.0f, 3.0f, wallT, COL_WOOD_DARK, 8.0f);
        unbind(shader);

        // ---- Sand/gravel berm behind Death Wall ----
        bindTex(shader, texGravel, 4.0f);
        setMaterial(shader, COL_GRAVEL, 0.22f, 0.92f, 0.02f, 2.0f);
        cube.draw(shader, I, deathWallCX - 2.5f, 0.0f, cwZ2 - 0.1f,
                  5.0f, 1.5f, 0.5f, COL_GRAVEL * 0.9f, 2.0f);
        unbind(shader);

        // ---- Courtyard gravel floor ----
        bindTex(shader, texGravel, 6.0f);
        plane.draw(shader, I, cwX1, 0.01f, cwZ1, cwX2 - cwX1, 1.0f, cwZ2 - cwZ1, COL_GRAVEL * 0.8f, 2.0f);
        unbind(shader);

        // ---- Wooden screens lining courtyard walls ----
        bindTex(shader, texWoodDark, 3.0f);
        setMaterial(shader, COL_WOOD_DARK, 0.15f, 0.70f, 0.05f, 6.0f);
        // Panels along west inner wall
        cube.draw(shader, I, cwX1 + wallT, 0.0f, cwZ1 + wallT,
                  0.1f, 2.5f, cwZ2 - cwZ1 - wallT * 2, COL_WOOD_DARK, 6.0f);
        // Panels along east inner wall
        cube.draw(shader, I, cwX2 - wallT - 0.1f, 0.0f, cwZ1 + wallT,
                  0.1f, 2.5f, cwZ2 - cwZ1 - wallT * 2, COL_WOOD_DARK, 6.0f);
        unbind(shader);
    }

private:
    static inline const glm::vec3 COL_BRICK_DARK = glm::vec3(0.35f, 0.17f, 0.05f);
    static inline const glm::vec3 COL_WOOD_DARK  = glm::vec3(0.30f, 0.18f, 0.08f);
    static inline const glm::vec3 COL_GRAVEL     = glm::vec3(0.40f, 0.38f, 0.35f);

    void bindTex(Shader& s, unsigned int tex, float rep) const {
        glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, tex);
        s.setInt("texture1", 0); s.setBool("useTexture", tex != 0); s.setFloat("texRepeat", rep);
    }
    void unbind(Shader& s) const { s.setBool("useTexture", false); }
    void setMaterial(Shader& s, glm::vec3 col, float a, float d, float sp, float sh) const {
        s.setVec3("material.ambient", col * a); s.setVec3("material.diffuse", col * d);
        s.setVec3("material.specular", glm::vec3(sp)); s.setFloat("material.shininess", sh);
        s.setVec3("material.emissive", glm::vec3(0.0f));
    }
};

#endif
