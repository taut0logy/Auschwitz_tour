#ifndef CREMATORYZONE_H
#define CREMATORYZONE_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <cmath>
#include "Shader.h"
#include "Flyweight.h"
#include "primitives/RuledSurface.h"
#include "primitives/Cube.h"
#include "primitives/Cylinder.h"
#include "primitives/Plane.h"

// ================================================================
// CrematoryZone: Crematorium I building, chimney, ruled-surface roof,
// furnace annex, gravel path, gallows
// Per Sections 2.6, 3.5, 7.5
// Located at X = -118, Z = -30
// ================================================================
class CrematoryZone {
public:
    MeshFlyweight ruledRoofMesh;

    void init() {
        buildRuledRoof();
    }

    void render(Shader& shader, const glm::mat4& I,
                Cube& cube, Cylinder& cyl, Plane& plane,
                unsigned int texBrickDark, unsigned int texRoofTile,
                unsigned int texConcrete, unsigned int texGravel,
                unsigned int texWoodDark) const
    {
        float cx = -118.0f, cz = -30.0f; // crematorium position

        // =============================================
        // Main building: 30m long × 11m wide × 4.5m tall
        // =============================================
        bindTex(shader, texBrickDark, 10.0f);
        setMaterial(shader, COL_BRICK_DARK, 0.06f, 0.65f, 0.03f, 4.0f);
        cube.draw(shader, I, cx - 15.0f, 0.0f, cz - 5.5f, 30.0f, 4.5f, 11.0f, COL_BRICK_DARK, 4.0f);

        // =============================================
        // Furnace antechamber: 6m × 6m × 3.5m on western end
        // =============================================
        cube.draw(shader, I, cx - 15.0f - 6.0f, 0.0f, cz - 3.0f, 6.0f, 3.5f, 6.0f, COL_BRICK_DARK, 4.0f);
        unbind(shader);

        // =============================================
        // Ruled surface roof (Bezier)
        // =============================================
        bindTex(shader, texRoofTile, 6.0f);
        setMaterial(shader, COL_ROOF_TILE, 0.13f, 0.72f, 0.06f, 8.0f);
        shader.setMat4("model", I);
        ruledRoofMesh.draw();
        unbind(shader);

        // =============================================
        // Chimney stack: 1.6 × 1.6 × 12m tall
        // At X = -125, Z = -32 (western end)
        // =============================================
        bindTex(shader, texBrickDark, 8.0f);
        setMaterial(shader, COL_BRICK_DARK, 0.06f, 0.65f, 0.03f, 4.0f);
        cube.draw(shader, I, -125.8f, 0.0f, -32.8f, 1.6f, 12.0f, 1.6f, COL_BRICK_DARK, 4.0f);
        unbind(shader);

        // =============================================
        // Gravel path: 2m wide surrounding building
        // =============================================
        bindTex(shader, texGravel, 8.0f);
        setMaterial(shader, COL_GRAVEL, 0.22f, 0.92f, 0.02f, 2.0f);
        // North side
        plane.draw(shader, I, cx - 17.0f, 0.005f, cz - 7.5f, 34.0f, 1.0f, 2.0f, COL_GRAVEL * 0.85f, 2.0f);
        // South side
        plane.draw(shader, I, cx - 17.0f, 0.005f, cz + 5.5f, 34.0f, 1.0f, 2.0f, COL_GRAVEL * 0.85f, 2.0f);
        // West side
        plane.draw(shader, I, cx - 23.0f, 0.005f, cz - 5.5f, 2.0f, 1.0f, 11.0f, COL_GRAVEL * 0.85f, 2.0f);
        // East side
        plane.draw(shader, I, cx + 15.0f, 0.005f, cz - 5.5f, 2.0f, 1.0f, 11.0f, COL_GRAVEL * 0.85f, 2.0f);
        unbind(shader);

        // =============================================
        // Gallows at X = -110, Z = -22
        // =============================================
        renderGallows(shader, I, cube, cyl, texWoodDark);
    }

    void cleanup() {
        ruledRoofMesh.cleanup();
    }

private:
    static inline const glm::vec3 COL_BRICK_DARK = glm::vec3(0.35f, 0.17f, 0.05f);
    static inline const glm::vec3 COL_ROOF_TILE  = glm::vec3(0.35f, 0.30f, 0.25f);
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

    void buildRuledRoof() {
        // Ruled surface between two parallel Bezier curves
        // Crematorium building spans X = [-133, -103], Z = [-35.5, -24.5]
        // Roof gable ends at Z = -35.5 and Z = -24.5
        float cx = -118.0f, cz = -30.0f;
        float halfLen = 15.0f, halfWid = 5.5f;

        glm::vec3 curveA[4] = {
            glm::vec3(cx - halfLen, 4.5f, cz - halfWid),
            glm::vec3(cx - halfLen * 0.3f, 5.5f, cz - halfWid),
            glm::vec3(cx + halfLen * 0.3f, 5.5f, cz - halfWid),
            glm::vec3(cx + halfLen, 4.5f, cz - halfWid)
        };
        glm::vec3 curveB[4] = {
            glm::vec3(cx - halfLen, 4.5f, cz + halfWid),
            glm::vec3(cx - halfLen * 0.3f, 5.5f, cz + halfWid),
            glm::vec3(cx + halfLen * 0.3f, 5.5f, cz + halfWid),
            glm::vec3(cx + halfLen, 4.5f, cz + halfWid)
        };

        std::vector<float> verts;
        std::vector<unsigned int> indices;
        RuledSurface::generate(curveA, curveB, 32, 8, verts, indices);
        ruledRoofMesh.initFromData(verts, indices);
    }

    void renderGallows(Shader& shader, const glm::mat4& I,
                       Cube& cube, Cylinder& cyl,
                       unsigned int texWoodDark) const
    {
        float gx = -110.0f, gz = -22.0f;

        bindTex(shader, texWoodDark, 3.0f);
        setMaterial(shader, COL_WOOD_DARK, 0.15f, 0.70f, 0.05f, 6.0f);

        // 1. Two vertical posts: 0.2×0.2×3.0m
        cube.draw(shader, I, gx - 1.1f, 0.0f, gz - 0.1f, 0.2f, 3.0f, 0.2f, COL_WOOD_DARK, 8.0f);
        cube.draw(shader, I, gx + 0.9f, 0.0f, gz - 0.1f, 0.2f, 3.0f, 0.2f, COL_WOOD_DARK, 8.0f);

        // 2. Horizontal crossbeam: 2.4×0.2×0.2m at Y=2.8
        cube.draw(shader, I, gx - 1.1f, 2.8f, gz - 0.1f, 2.2f, 0.2f, 0.2f, COL_WOOD_DARK, 8.0f);

        // 3. Rope: thin cylinder hanging 0.8m from crossbeam centre
        cyl.draw(shader, I, gx - 0.02f, 2.0f, gz - 0.02f, 0.04f, 0.8f, 0.04f, COL_WOOD_DARK * 0.8f, 4.0f);

        // 4. Noose: approximated as a small torus (ring of tiny cylinders)
        float nooseR = 0.08f;
        int nooseSegs = 12;
        for (int i = 0; i < nooseSegs; i++) {
            float angle = (float)i / nooseSegs * 2.0f * 3.14159f;
            float nx = gx + nooseR * cosf(angle);
            float nz = gz + nooseR * sinf(angle);
            cube.draw(shader, I, nx - 0.015f, 1.92f, nz - 0.015f,
                      0.03f, 0.06f, 0.03f, COL_WOOD_DARK * 0.7f, 4.0f);
        }

        // 5. Platform step: 1.0×0.25×0.6m
        cube.draw(shader, I, gx - 0.5f, 0.0f, gz - 0.3f, 1.0f, 0.25f, 0.6f, COL_WOOD_DARK, 8.0f);

        unbind(shader);
    }
};

#endif
