#ifndef GUARDTOWERS_H
#define GUARDTOWERS_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <cmath>
#include "Shader.h"
#include "primitives/Cube.h"
#include "primitives/Cylinder.h"

// ================================================================
// GuardTowers: 12 guard towers — 9-component geometry per tower
// ================================================================
class GuardTowers {
public:
    struct TowerPos { float x, z; };

    // 12 tower positions (2.5m outside outer fence at +/-162/±95)
    static constexpr int NUM_TOWERS = 12;

    // Corner towers: just outside corners of outer fence
    // Mid-perimeter: evenly distributed along fence lines
    TowerPos towers[NUM_TOWERS] = {
        // 4 corners
        { 164.5f, -97.5f}, { 164.5f,  97.5f},
        {-164.5f, -97.5f}, {-164.5f,  97.5f},
        // North fence mid-points (3 towers)
        { 45.0f, -97.5f}, {-45.0f, -97.5f}, { 0.0f, -97.5f},
        // South fence mid-points
        { 45.0f,  97.5f}, {-45.0f,  97.5f},
        // East (around gate) and West fence mid-points
        { 164.5f,  15.0f}, {-164.5f,  0.0f},
        // Above gate on East
        { 164.5f, -15.0f}
    };

    void render(Shader& shader, const glm::mat4& I,
                Cube& cube, Cylinder& cyl,
                unsigned int texWoodPlank, unsigned int texRoofTile,
                unsigned int texMetalGrey, unsigned int texConcrete) const
    {
        for (int i = 0; i < NUM_TOWERS; i++) {
            renderTower(shader, I, cube, cyl,
                towers[i].x, towers[i].z,
                texWoodPlank, texRoofTile, texMetalGrey, texConcrete, i);
        }
    }

private:
    static inline const glm::vec3 COL_WOOD       = glm::vec3(0.45f, 0.30f, 0.15f);
    static inline const glm::vec3 COL_WOOD_DARK  = glm::vec3(0.30f, 0.18f, 0.08f);
    static inline const glm::vec3 COL_ROOF_TILE  = glm::vec3(0.35f, 0.30f, 0.25f);
    static inline const glm::vec3 COL_METAL      = glm::vec3(0.50f, 0.48f, 0.46f);
    static inline const glm::vec3 COL_CONCRETE   = glm::vec3(0.60f, 0.58f, 0.55f);

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

    void renderTower(Shader& shader, const glm::mat4& I,
                     Cube& cube, Cylinder& cyl,
                     float tx, float tz,
                     unsigned int texWoodPlank, unsigned int texRoofTile,
                     unsigned int texMetalGrey, unsigned int texConcrete,
                     int towerIndex) const
    {
        // ---- 1. Foundation pad: 4.5 × 4.5 × 0.3m concrete ----
        bindTex(shader, texConcrete, 4.0f);
        setMaterial(shader, COL_CONCRETE, 0.18f, 0.82f, 0.06f, 6.0f);
        cube.draw(shader, I, tx - 2.25f, -0.3f, tz - 2.25f, 4.5f, 0.3f, 4.5f, COL_CONCRETE, 6.0f);
        unbind(shader);

        // ---- 2. Four corner legs: cylinder r=0.15m, h=4.8m ----
        bindTex(shader, texWoodPlank, 3.0f);
        setMaterial(shader, COL_WOOD, 0.18f, 0.78f, 0.08f, 8.0f);
        float legOff = 1.6f;
        for (float dx : {-legOff, legOff}) {
            for (float dz : {-legOff, legOff}) {
                cyl.draw(shader, I, tx + dx - 0.15f, 0.0f, tz + dz - 0.15f,
                         0.30f, 4.8f, 0.30f, COL_WOOD, 8.0f);
            }
        }

        // ---- 3. Cross-bracing at Y=1.5 and Y=3.0 ----
        for (float y : {1.5f, 3.0f}) {
            // X-braces between adjacent legs (simplified as thin boxes)
            // North-south braces
            cube.draw(shader, I, tx - legOff - 0.03f, y, tz - legOff,
                      0.06f, 0.06f, legOff * 2, COL_WOOD, 8.0f);
            cube.draw(shader, I, tx + legOff - 0.03f, y, tz - legOff,
                      0.06f, 0.06f, legOff * 2, COL_WOOD, 8.0f);
            // East-west braces
            cube.draw(shader, I, tx - legOff, y, tz - legOff - 0.03f,
                      legOff * 2, 0.06f, 0.06f, COL_WOOD, 8.0f);
            cube.draw(shader, I, tx - legOff, y, tz + legOff - 0.03f,
                      legOff * 2, 0.06f, 0.06f, COL_WOOD, 8.0f);
        }

        // ---- 4. Platform floor: 4.8 × 0.2 × 4.8 at Y=4.8 ----
        cube.draw(shader, I, tx - 2.4f, 4.8f, tz - 2.4f, 4.8f, 0.2f, 4.8f, COL_WOOD, 8.0f);

        // ---- 5. Platform railing ----
        float railY[] = {5.3f, 5.7f, 5.9f};
        for (int r = 0; r < 3; r++) {
            // Four sides of railing
            cube.draw(shader, I, tx - 2.4f, railY[r], tz - 2.4f, 4.8f, 0.06f, 0.06f, COL_WOOD, 8.0f);
            cube.draw(shader, I, tx - 2.4f, railY[r], tz + 2.34f, 4.8f, 0.06f, 0.06f, COL_WOOD, 8.0f);
            cube.draw(shader, I, tx - 2.4f, railY[r], tz - 2.4f, 0.06f, 0.06f, 4.8f, COL_WOOD, 8.0f);
            cube.draw(shader, I, tx + 2.34f, railY[r], tz - 2.4f, 0.06f, 0.06f, 4.8f, COL_WOOD, 8.0f);
        }
        // Balusters (5 per face)
        for (int face = 0; face < 4; face++) {
            for (int b = 0; b < 5; b++) {
                float off = -2.0f + b * 1.0f;
                float bx, bz;
                if (face == 0)      { bx = tx + off; bz = tz - 2.4f; }
                else if (face == 1) { bx = tx + off; bz = tz + 2.34f; }
                else if (face == 2) { bx = tx - 2.4f; bz = tz + off; }
                else                { bx = tx + 2.34f; bz = tz + off; }
                cyl.draw(shader, I, bx - 0.03f, 5.0f, bz - 0.03f, 0.06f, 0.9f, 0.06f, COL_WOOD, 8.0f);
            }
        }

        // ---- 6. Cabin: 3.0 × 2.2 × 2.4 at Y=5.0 ----
        cube.draw(shader, I, tx - 1.5f, 5.0f, tz - 1.2f, 3.0f, 2.2f, 2.4f, COL_WOOD, 8.0f);

        // Cabin window openings (simplified as dark recesses)
        shader.setVec3("material.emissive", glm::vec3(0.0f));
        for (int face = 0; face < 4; face++) {
            for (int w = 0; w < 2; w++) {
                float wx, wz;
                if (face == 0)      { wx = tx - 0.8f + w * 1.1f; wz = tz - 1.21f; }
                else if (face == 1) { wx = tx - 0.8f + w * 1.1f; wz = tz + 1.11f; }
                else if (face == 2) { wx = tx - 1.51f; wz = tz - 0.5f + w * 1.0f; }
                else                { wx = tx + 1.41f; wz = tz - 0.5f + w * 1.0f; }
                cube.draw(shader, I, wx, 5.8f, wz, 0.5f, 0.5f, 0.1f, glm::vec3(0.05f), 4.0f);
            }
        }
        unbind(shader);

        // ---- 7. Hip roof ----
        bindTex(shader, texRoofTile, 4.0f);
        setMaterial(shader, COL_ROOF_TILE, 0.13f, 0.72f, 0.06f, 8.0f);
        // Approximated as stepped boxes converging to a point
        cube.draw(shader, I, tx - 1.9f, 7.2f, tz - 1.6f, 3.8f, 0.25f, 3.2f, COL_ROOF_TILE, 8.0f);
        cube.draw(shader, I, tx - 1.4f, 7.45f, tz - 1.1f, 2.8f, 0.25f, 2.2f, COL_ROOF_TILE, 8.0f);
        cube.draw(shader, I, tx - 0.8f, 7.7f, tz - 0.6f, 1.6f, 0.2f, 1.2f, COL_ROOF_TILE, 8.0f);
        cube.draw(shader, I, tx - 0.3f, 7.9f, tz - 0.2f, 0.6f, 0.1f, 0.4f, COL_ROOF_TILE, 8.0f);
        unbind(shader);

        // ---- 8. Searchlight housing ----
        bindTex(shader, texMetalGrey, 2.0f);
        setMaterial(shader, COL_METAL, 0.10f, 0.50f, 0.80f, 64.0f);
        cube.draw(shader, I, tx - 0.2f, 7.2f, tz - 1.45f, 0.4f, 0.3f, 0.5f, COL_METAL, 64.0f);
        unbind(shader);

        // ---- 9. Access ladder ----
        bindTex(shader, texWoodPlank, 2.0f);
        setMaterial(shader, COL_WOOD_DARK, 0.15f, 0.70f, 0.05f, 6.0f);
        // Two ladder rails
        cyl.draw(shader, I, tx + 1.7f, 0.0f, tz - 1.2f, 0.06f, 4.8f, 0.06f, COL_WOOD_DARK, 6.0f);
        cyl.draw(shader, I, tx + 1.7f, 0.0f, tz - 0.9f, 0.06f, 4.8f, 0.06f, COL_WOOD_DARK, 6.0f);
        // Rungs (every 0.35m)
        for (float y = 0.35f; y < 4.8f; y += 0.35f) {
            cube.draw(shader, I, tx + 1.68f, y, tz - 1.2f, 0.04f, 0.04f, 0.3f, COL_WOOD_DARK, 6.0f);
        }
        unbind(shader);
    }
};

#endif
