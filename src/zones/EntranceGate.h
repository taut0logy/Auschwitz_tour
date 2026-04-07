#ifndef ENTRANCEGATE_H
#define ENTRANCEGATE_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <vector>
#include <cmath>
#include "Shader.h"
#include "Flyweight.h"
#include "BezierCurve.h"
#include "primitives/BezierTube.h"
#include "primitives/Cube.h"
#include "primitives/Cylinder.h"
#include "primitives/Plane.h"

// ================================================================
// EntranceGate: Gatehouse building, Bezier arch, gate leaves,
// text billboard, flagpoles, sentry boxes
// Per Sections 2.3, 3.6, 7.1
//
// REPOSITIONED: Moved from X=135 to X=+155 to create proper spacing
// from barracks grid (easternmost column at X=+112 with 40m blocks)
// ================================================================
class EntranceGate {
public:
    // Main gate position - moved east to create coherent camp layout
    static constexpr float GATE_X = 155.0f;  // Was 135.0f
    static constexpr float GATE_Z = 0.0f;
    
    // Building dimensions
    static constexpr float BUILDING_WIDTH_Z = 18.0f;  // Total Z span
    static constexpr float BUILDING_DEPTH_X = 12.0f;  // X depth
    static constexpr float BUILDING_HEIGHT = 5.5f;
    static constexpr float TUNNEL_WIDTH = 4.0f;
    static constexpr float TUNNEL_HEIGHT = 4.2f;

    MeshFlyweight archMesh;
    MeshFlyweight textBillboard;

    void init() {
        buildArchMesh();
        buildTextBillboard();
    }

    void render(Shader& shader, const glm::mat4& I,
                Cube& cube, Cylinder& cyl, Plane& plane,
                unsigned int texBrickDark, unsigned int texBrickRed,
                unsigned int texRoofTile, unsigned int texMetalIron,
                unsigned int texCobblestone, unsigned int texWoodDark,
                unsigned int texTextArbeit) const
    {
        float gateX = GATE_X;
        float gateZ = GATE_Z;

        // =============================================
        // GATEHOUSE BUILDING
        // 18m wide (Z) × 12m deep (X) × 5.5m tall
        // Entry tunnel: 4m wide × 4.2m tall at Z = 0
        // =============================================

        // Building SW corner
        float bldgX = gateX - 6.0f;  // 12m depth, centred at gateX
        float bldgZ = gateZ - 9.0f;  // 18m width

        // South wing (Z = gateZ+2 to gateZ+9)
        bindTex(shader, texBrickDark, 6.0f);
        setMaterial(shader, COL_BRICK_DARK, 0.08f, 0.65f, 0.03f, 4.0f);
        cube.draw(shader, I, bldgX, 0.0f, gateZ + 2.0f, 12.0f, 5.5f, 7.0f, COL_BRICK_DARK, 4.0f);

        // North wing (Z = gateZ-9 to gateZ-2)
        cube.draw(shader, I, bldgX, 0.0f, gateZ - 9.0f, 12.0f, 5.5f, 7.0f, COL_BRICK_DARK, 4.0f);

        // Top span over tunnel (4m wide gap, so walls on either side of Z = -2..+2)
        cube.draw(shader, I, bldgX, 4.2f, gateZ - 2.0f, 12.0f, 1.3f, 4.0f, COL_BRICK_DARK, 4.0f);

        // Tunnel walls (sides of the 4m-wide passage)
        cube.draw(shader, I, bldgX, 0.0f, gateZ - 2.0f, 12.0f, 4.2f, 0.3f, COL_BRICK_DARK, 4.0f);
        cube.draw(shader, I, bldgX, 0.0f, gateZ + 1.7f, 12.0f, 4.2f, 0.3f, COL_BRICK_DARK, 4.0f);
        unbind(shader);

        // Tunnel floor (cobblestone)
        bindTex(shader, texCobblestone, 8.0f);
        setMaterial(shader, glm::vec3(0.5f), 0.18f, 0.78f, 0.04f, 8.0f);
        plane.draw(shader, I, bldgX, 0.01f, gateZ - 1.7f, 12.0f, 1.0f, 3.4f, glm::vec3(0.45f), 8.0f);
        unbind(shader);

        // Windows on gatehouse south face (4 windows)
        bindTex(shader, texWoodDark, 2.0f);
        setMaterial(shader, COL_WOOD_DARK, 0.15f, 0.70f, 0.05f, 6.0f);
        for (int w = 0; w < 4; w++) {
            float winZ = gateZ + 3.0f + w * 1.5f;
            // Window frame on south face of south wing
            cube.draw(shader, I, bldgX + 6.0f - 0.5f, 2.0f, winZ, 0.08f, 1.2f, 1.0f, COL_WOOD_DARK, 6.0f);
        }
        unbind(shader);

        // =============================================
        // ROOFS (low hipped roof over each wing)
        // ridge at Y=7.5m
        // =============================================
        bindTex(shader, texRoofTile, 8.0f);
        setMaterial(shader, COL_ROOF_TILE, 0.13f, 0.72f, 0.06f, 8.0f);
        // South wing roof (flat approximation)
        cube.draw(shader, I, bldgX - 0.5f, 5.5f, gateZ + 1.5f, 13.0f, 0.3f, 8.0f, COL_ROOF_TILE, 8.0f);
        cube.draw(shader, I, bldgX + 0.5f, 5.8f, gateZ + 2.5f, 11.0f, 0.3f, 6.0f, COL_ROOF_TILE, 8.0f);
        cube.draw(shader, I, bldgX + 1.5f, 6.1f, gateZ + 3.5f, 9.0f, 0.3f, 4.0f, COL_ROOF_TILE, 8.0f);
        // North wing roof
        cube.draw(shader, I, bldgX - 0.5f, 5.5f, gateZ - 9.5f, 13.0f, 0.3f, 8.0f, COL_ROOF_TILE, 8.0f);
        cube.draw(shader, I, bldgX + 0.5f, 5.8f, gateZ - 8.5f, 11.0f, 0.3f, 6.0f, COL_ROOF_TILE, 8.0f);
        cube.draw(shader, I, bldgX + 1.5f, 6.1f, gateZ - 7.5f, 9.0f, 0.3f, 4.0f, COL_ROOF_TILE, 8.0f);
        unbind(shader);

        // =============================================
        // GATE ARCH (Bezier) — sits at east face of gatehouse
        // =============================================
        bindTex(shader, texMetalIron, 2.0f);
        setMaterial(shader, COL_METAL, 0.08f, 0.45f, 0.95f, 96.0f);

        // Left vertical bar
        cube.draw(shader, I, gateX + 5.5f, 0.0f, gateZ - 1.8f, 0.08f, 3.5f, 0.08f, COL_METAL, 96.0f);
        // Right vertical bar
        cube.draw(shader, I, gateX + 5.5f, 0.0f, gateZ + 1.72f, 0.08f, 3.5f, 0.08f, COL_METAL, 96.0f);

        // Horizontal intermediate bars at Y = 1.0, 2.0, 3.0
        for (float y : {1.0f, 2.0f, 3.0f}) {
            cube.draw(shader, I, gateX + 5.5f, y, gateZ - 1.8f, 0.06f, 0.06f, 3.6f, COL_METAL, 96.0f);
        }

        // Vertical decorative bars (12 evenly spaced)
        for (int i = 1; i <= 12; i++) {
            float z = gateZ - 1.8f + (float)i * 3.6f / 13.0f;
            cube.draw(shader, I, gateX + 5.52f, 0.0f, z, 0.03f, 3.5f, 0.03f, COL_METAL, 96.0f);
        }

        // Bezier arch tube
        glm::mat4 archModel = glm::translate(I, glm::vec3(gateX + 5.54f, 0.0f, 0.0f));
        shader.setMat4("model", archModel);
        archMesh.draw();

        unbind(shader);

        // =============================================
        // "ARBEIT MACHT FREI" text billboard
        // Attached at Y = 3.0m on the arch
        // =============================================
        if (texTextArbeit) {
            bindTex(shader, texTextArbeit, 1.0f);
            shader.setVec3("material.ambient", glm::vec3(0.3f));
            shader.setVec3("material.diffuse", glm::vec3(0.8f));
            shader.setVec3("material.specular", glm::vec3(0.1f));
            shader.setFloat("material.shininess", 4.0f);
            // Flat quad at arch bar level
            cube.draw(shader, I, gateX + 5.4f, 2.85f, gateZ - 1.6f, 0.02f, 0.35f, 3.2f, glm::vec3(0.9f), 4.0f);
            unbind(shader);
        }

        // =============================================
        // GATE LEAVES (two panels, rendered open at 90°)
        // =============================================
        bindTex(shader, texMetalIron, 3.0f);
        setMaterial(shader, COL_METAL, 0.08f, 0.45f, 0.95f, 96.0f);
        // Left gate leaf (rotated 90° flush with south wall)
        cube.draw(shader, I, gateX + 5.5f, 0.0f, gateZ - 2.0f, 0.06f, 3.4f, 0.06f, COL_METAL, 96.0f);
        // Grid members for left leaf (simplified)
        for (float y : {0.5f, 1.2f, 1.9f, 2.6f}) {
            cube.draw(shader, I, gateX + 5.5f, y, gateZ - 2.0f - 1.8f, 0.06f, 0.06f, 1.8f, COL_METAL, 96.0f);
        }
        for (int i = 0; i < 4; i++) {
            float z = gateZ - 2.0f - 0.45f * (i + 1);
            cube.draw(shader, I, gateX + 5.52f, 0.0f, z, 0.04f, 3.4f, 0.04f, COL_METAL, 96.0f);
        }
        // Right gate leaf (mirror)
        for (float y : {0.5f, 1.2f, 1.9f, 2.6f}) {
            cube.draw(shader, I, gateX + 5.5f, y, gateZ + 2.0f, 0.06f, 0.06f, 1.8f, COL_METAL, 96.0f);
        }
        for (int i = 0; i < 4; i++) {
            float z = gateZ + 2.0f + 0.45f * (i + 1);
            cube.draw(shader, I, gateX + 5.52f, 0.0f, z, 0.04f, 3.4f, 0.04f, COL_METAL, 96.0f);
        }
        unbind(shader);

        // =============================================
        // SENTRY BOXES (2m × 2m × 2.4m at Z = ±4, X = +140)
        // =============================================
        bindTex(shader, texBrickDark, 3.0f);
        setMaterial(shader, COL_BRICK_DARK, 0.08f, 0.65f, 0.03f, 4.0f);
        cube.draw(shader, I, 140.0f, 0.0f, gateZ + 3.0f, 2.0f, 2.4f, 2.0f, COL_BRICK_DARK, 4.0f);
        cube.draw(shader, I, 140.0f, 0.0f, gateZ - 5.0f, 2.0f, 2.4f, 2.0f, COL_BRICK_DARK, 4.0f);
        unbind(shader);

        // =============================================
        // FLAGPOLES (2 steel poles, 8m tall, r = 0.15m)
        // At X = +132, Z = ±6
        // =============================================
        setMaterial(shader, COL_METAL, 0.10f, 0.50f, 0.80f, 64.0f);
        cyl.draw(shader, I, 132.0f, 0.0f, gateZ + 6.0f, 0.15f, 8.0f, 0.15f, COL_METAL, 64.0f);
        cyl.draw(shader, I, 132.0f, 0.0f, gateZ - 6.0f, 0.15f, 8.0f, 0.15f, COL_METAL, 64.0f);
    }

    void cleanup() {
        archMesh.cleanup();
        textBillboard.cleanup();
    }

private:
    static inline const glm::vec3 COL_BRICK_DARK  = glm::vec3(0.35f, 0.17f, 0.05f);
    static inline const glm::vec3 COL_ROOF_TILE   = glm::vec3(0.35f, 0.30f, 0.25f);
    static inline const glm::vec3 COL_METAL        = glm::vec3(0.50f, 0.48f, 0.46f);
    static inline const glm::vec3 COL_WOOD_DARK   = glm::vec3(0.30f, 0.18f, 0.08f);

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

    void buildArchMesh() {
        // Gate arch Bezier: spans Z = -1.8 to +1.8 at Y = 3.5 (top of vertical bars)
        // Defined in YZ plane, arch rises to Y = 5.2
        glm::vec3 P0(0.0f, 3.5f, -1.8f);
        glm::vec3 P1(0.0f, 5.2f, -1.8f);
        glm::vec3 P2(0.0f, 5.2f,  1.8f);
        glm::vec3 P3(0.0f, 3.5f,  1.8f);

        std::vector<float> verts;
        std::vector<unsigned int> indices;
        BezierTube::generate(P0, P1, P2, P3, 0.06f, 32, 8, verts, indices);
        archMesh.initFromData(verts, indices);
    }

    void buildTextBillboard() {
        // Simple quad for "Arbeit Macht Frei" text
        // Built into the render method using cube primitive instead
    }
};

#endif
