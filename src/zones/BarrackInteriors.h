#ifndef BARRACKINTERIORS_H
#define BARRACKINTERIORS_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include "Shader.h"
#include "Camera.h"
#include "primitives/Cube.h"
#include "primitives/Cylinder.h"
#include "primitives/Sphere.h"
#include "primitives/Plane.h"
#include "zones/BarrackGrid.h"

// ================================================================
// BarrackInteriors: 3 interior types, proximity-gated rendering
// Type A: Sleeping quarters (most blocks)
// Type B: Washroom/utility (Blocks 10, 22)
// Type C: Block 11 punishment block
// Per Sections 3.2, 11
// ================================================================
class BarrackInteriors {
public:
    static constexpr float INTERIOR_DRAW_DIST = 25.0f;
    static constexpr float GLOW_DRAW_DIST     = 50.0f;

    void render(Shader& shader, const glm::mat4& I,
                Cube& cube, Cylinder& cyl, Sphere& sphere, Plane& plane,
                const Camera& camera,
                unsigned int texWoodPlank, unsigned int texBrickDark,
                unsigned int texStoneFloor, unsigned int texPlasterWhite,
                unsigned int texStrawBedding, unsigned int texBlackMetal,
                unsigned int texTileGrey, unsigned int texRoofTile) const
    {
        for (int blockNum = 1; blockNum <= 24; blockNum++) {
            glm::vec3 blockCentre = BarrackGrid::getBlockCentre(blockNum);
            float dist = glm::length(glm::vec3(camera.position.x, 0, camera.position.z) -
                                     glm::vec3(blockCentre.x, 0, blockCentre.z));

            if (dist < INTERIOR_DRAW_DIST) {
                int type = getInteriorType(blockNum);
                glm::vec3 sw = BarrackGrid::getBlockSW(blockNum);

                if (type == 0)
                    renderTypeA(shader, I, cube, cyl, sphere, plane, sw,
                                texWoodPlank, texRoofTile, texStrawBedding, texBlackMetal);
                else if (type == 1)
                    renderTypeB(shader, I, cube, cyl, plane, sw,
                                texTileGrey, texPlasterWhite);
                else
                    renderTypeC(shader, I, cube, plane, sw,
                                texBrickDark, texStoneFloor);
            }
            else if (dist < GLOW_DRAW_DIST) {
                // Window glow quads (visible from outside)
                renderWindowGlow(shader, I, cube, blockNum);
            }
        }
    }

    // Render emissive bulbs inside barracks (call with unlit shader)
    void renderBulbs(Shader& unlitShader, const glm::mat4& I,
                     Sphere& sphere, const Camera& camera) const
    {
        for (int blockNum = 1; blockNum <= 24; blockNum++) {
            glm::vec3 blockCentre = BarrackGrid::getBlockCentre(blockNum);
            float dist = glm::length(camera.position - blockCentre);
            if (dist < INTERIOR_DRAW_DIST) {
                glm::vec3 sw = BarrackGrid::getBlockSW(blockNum);
                // 4 bulbs per block at X = 8, 16, 24, 32; elevated for single high ceiling
                for (float lx : {8.0f, 16.0f, 24.0f, 32.0f}) {
                    sphere.drawEmissive(unlitShader, I,
                        sw.x + lx - 0.04f, 7.0f, sw.z + 5.5f - 0.04f,
                        0.08f, 0.08f, 0.08f,
                        glm::vec3(1.0f, 0.93f, 0.53f)); // #FFEE88
                }
            }
        }
    }

private:
    static inline const glm::vec3 COL_WOOD      = glm::vec3(0.45f, 0.30f, 0.15f);
    static inline const glm::vec3 COL_WOOD_DARK = glm::vec3(0.30f, 0.18f, 0.08f);
    static inline const glm::vec3 COL_BRICK_DARK = glm::vec3(0.35f, 0.17f, 0.05f);
    static inline const glm::vec3 COL_METAL     = glm::vec3(0.15f, 0.15f, 0.15f);
    static inline const glm::vec3 COL_TILE      = glm::vec3(0.55f, 0.55f, 0.53f);
    static inline const glm::vec3 COL_ROOF_TILE  = glm::vec3(0.35f, 0.30f, 0.25f);

    int getInteriorType(int blockNum) const {
        if (blockNum == 11) return 2;           // Type C (punishment)
        if (blockNum == 10 || blockNum == 22) return 1; // Type B (washroom)
        return 0;                                // Type A (sleeping)
    }

    void bindTex(Shader& s, unsigned int tex, float rep) const {
        glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, tex);
        s.setInt("texture1", 0); s.setBool("useTexture", tex != 0); s.setFloat("texRepeat", rep);
    }
    void unbind(Shader& s) const { s.setBool("useTexture", false); }
    void setMat(Shader& s, glm::vec3 col, float a, float d, float sp, float sh) const {
        s.setVec3("material.ambient", col * a); s.setVec3("material.diffuse", col * d);
        s.setVec3("material.specular", glm::vec3(sp)); s.setFloat("material.shininess", sh);
        s.setVec3("material.emissive", glm::vec3(0.0f));
    }

    // ================================================================
    // TYPE A — Sleeping Quarters
    // ================================================================
    void renderTypeA(Shader& shader, const glm::mat4& I,
                     Cube& cube, Cylinder& cyl, Sphere& sphere, Plane& plane,
                     const glm::vec3& sw,
                     unsigned int texWoodPlank, unsigned int texRoofTile,
                     unsigned int texStraw, unsigned int texBlackMetal) const
    {
        float fx = sw.x + 0.75f, fz = sw.z + 0.5f; // interior offset

        // Floor
        bindTex(shader, texWoodPlank, 20.0f);
        setMat(shader, COL_WOOD, 0.18f, 0.78f, 0.08f, 8.0f);
        plane.draw(shader, I, fx, 0.02f, fz, 38.5f, 1.0f, 11.0f, COL_WOOD, 8.0f);

        // Central support columns (7 columns at centreline)
        bindTex(shader, texWoodPlank, 3.0f);
        for (int c = 0; c < 7; c++) {
            float cx = fx + 5.0f + c * 5.0f;
            // Extend columns to the roof ridge (8m wall height + 2.5m roof rise = ~10.5m)
            cube.draw(shader, I, cx - 0.2f, 0.0f, fz + 5.3f, 0.4f, 10.5f, 0.4f, COL_WOOD * 0.8f, 8.0f);
        }

        // Triple-tier bunk beds (10 per side)
        for (int side = 0; side < 2; side++) {
            float bz = (side == 0) ? fz + 0.5f : fz + 8.5f;
            for (int b = 0; b < 10; b++) {
                float bx = fx + 1.5f + b * 3.8f;
                renderBunkBed(shader, I, cube, cyl, bx, bz, texWoodPlank, texStraw);
            }
        }

        // Heating stove
        bindTex(shader, texBlackMetal, 2.0f);
        setMat(shader, COL_METAL, 0.05f, 0.30f, 0.60f, 32.0f);
        cube.draw(shader, I, fx + 20.0f - 0.4f, 0.0f, fz + 5.5f, 0.8f, 1.2f, 0.8f, COL_METAL, 32.0f);
        // Firebox door (emissive orange glow)
        shader.setVec3("material.emissive", glm::vec3(0.8f, 0.4f, 0.0f));
        cube.draw(shader, I, fx + 20.0f - 0.41f, 0.3f, fz + 5.7f, 0.02f, 0.25f, 0.2f, glm::vec3(0.9f, 0.5f, 0.1f), 4.0f);
        shader.setVec3("material.emissive", glm::vec3(0.0f));
        unbind(shader);

        // Small table near stove
        bindTex(shader, texWoodPlank, 2.0f);
        setMat(shader, COL_WOOD, 0.18f, 0.78f, 0.08f, 8.0f);
        // Table top
        cube.draw(shader, I, fx + 22.0f, 0.65f, fz + 5.0f, 1.5f, 0.05f, 0.75f, COL_WOOD, 8.0f);
        // Table legs
        for (float lx : {0.0f, 1.4f}) {
            for (float lz : {0.0f, 0.65f}) {
                cube.draw(shader, I, fx + 22.0f + lx, 0.0f, fz + 5.0f + lz, 0.06f, 0.65f, 0.06f, COL_WOOD * 0.8f, 8.0f);
            }
        }
        // Benches
        cube.draw(shader, I, fx + 22.2f, 0.35f, fz + 4.3f, 1.2f, 0.05f, 0.25f, COL_WOOD * 0.9f, 8.0f);
        cube.draw(shader, I, fx + 22.2f, 0.35f, fz + 5.85f, 1.2f, 0.05f, 0.25f, COL_WOOD * 0.9f, 8.0f);
        unbind(shader);
    }

    void renderBunkBed(Shader& shader, const glm::mat4& I,
                       Cube& cube, Cylinder& cyl,
                       float bx, float bz,
                       unsigned int texWood, unsigned int texStraw) const
    {
        // 4 vertical posts
        for (float px : {0.0f, 1.9f}) {
            for (float pz : {0.0f, 0.84f}) {
                cube.draw(shader, I, bx + px, 0.0f, bz + pz, 0.06f, 1.9f, 0.06f, COL_WOOD * 0.7f, 8.0f);
            }
        }
        // 3 horizontal bed platforms (tiers at Y=0.3, 1.0, 1.7)
        for (float ty : {0.3f, 1.0f, 1.7f}) {
            cube.draw(shader, I, bx, ty, bz, 2.0f, 0.06f, 0.9f, COL_WOOD * 0.8f, 8.0f);
            // Straw bedding overlay
            if (texStraw) {
                bindTex(shader, texStraw, 2.0f);
                cube.draw(shader, I, bx + 0.05f, ty + 0.06f, bz + 0.05f,
                          1.9f, 0.03f, 0.8f, glm::vec3(0.7f, 0.6f, 0.35f), 4.0f);
                bindTex(shader, texWood, 3.0f);
            }
        }
        // Ladder rungs
        for (float ry : {0.5f, 0.85f}) {
            cyl.draw(shader, I, bx + 1.9f, ry, bz + 0.1f, 0.04f, 0.06f, 0.7f, COL_WOOD * 0.6f, 8.0f);
        }
    }

    // ================================================================
    // TYPE B — Washroom/Utility
    // ================================================================
    void renderTypeB(Shader& shader, const glm::mat4& I,
                     Cube& cube, Cylinder& cyl, Plane& plane,
                     const glm::vec3& sw,
                     unsigned int texTileGrey, unsigned int texPlaster) const
    {
        float fx = sw.x + 0.75f, fz = sw.z + 0.5f;

        // Floor (tile)
        bindTex(shader, texTileGrey, 20.0f);
        setMat(shader, COL_TILE, 0.20f, 0.80f, 0.06f, 16.0f);
        plane.draw(shader, I, fx, 0.02f, fz, 38.5f, 1.0f, 11.0f, COL_TILE, 16.0f);
        unbind(shader);

        // Long concrete trough along centreline
        shader.setBool("useTexture", false);
        setMat(shader, glm::vec3(0.5f), 0.18f, 0.82f, 0.06f, 8.0f);
        cube.draw(shader, I, fx + 1.5f, 0.0f, fz + 5.0f, 35.0f, 0.8f, 0.5f, glm::vec3(0.55f), 8.0f);

        // 10 tap fixtures (small cylinders)
        for (int t = 0; t < 10; t++) {
            float tx = fx + 3.0f + t * 3.5f;
            cyl.draw(shader, I, tx, 0.8f, fz + 5.1f, 0.05f, 0.3f, 0.05f, glm::vec3(0.6f), 32.0f);
            // Tap arm
            cube.draw(shader, I, tx - 0.03f, 1.0f, fz + 5.0f, 0.06f, 0.06f, 0.15f, glm::vec3(0.6f), 32.0f);
        }

        // Latrine bench along south wall
        cube.draw(shader, I, fx + 2.0f, 0.0f, fz + 9.0f, 34.0f, 0.5f, 1.5f, COL_WOOD * 0.7f, 8.0f);
    }

    // ================================================================
    // TYPE C — Block 11 Punishment
    // ================================================================
    void renderTypeC(Shader& shader, const glm::mat4& I,
                     Cube& cube, Plane& plane,
                     const glm::vec3& sw,
                     unsigned int texBrickDark, unsigned int texStoneFloor) const
    {
        float fx = sw.x + 0.75f, fz = sw.z + 0.5f;

        // Floor (dark stone)
        bindTex(shader, texStoneFloor, 15.0f);
        setMat(shader, glm::vec3(0.25f), 0.10f, 0.60f, 0.03f, 8.0f);
        plane.draw(shader, I, fx, 0.02f, fz, 38.5f, 1.0f, 11.0f, glm::vec3(0.25f), 8.0f);
        unbind(shader);

        // Central corridor (1.5m wide)
        bindTex(shader, texBrickDark, 6.0f);
        setMat(shader, COL_BRICK_DARK, 0.03f, 0.40f, 0.02f, 4.0f);

        // Cell walls on both sides (8 cells per side)
        for (int side = 0; side < 2; side++) {
            float cellZ = (side == 0) ? fz + 0.5f : fz + 6.5f;
            for (int c = 0; c < 8; c++) {
                float cellX = fx + 2.0f + c * 4.5f;
                float cellW, cellD;

                // Standing cells (cells 0-3 on north side): tiny 0.9x0.9m
                if (side == 0 && c < 4) {
                    cellW = 0.9f; cellD = 0.9f;
                } else {
                    cellW = 2.5f; cellD = 2.5f;
                }

                // Cell walls (3 sides — open side faces corridor)
                cube.draw(shader, I, cellX, 0.0f, cellZ, 0.15f, 2.8f, cellD, COL_BRICK_DARK * 0.7f, 4.0f);
                cube.draw(shader, I, cellX + cellW, 0.0f, cellZ, 0.15f, 2.8f, cellD, COL_BRICK_DARK * 0.7f, 4.0f);
                cube.draw(shader, I, cellX, 0.0f, cellZ + cellD - 0.15f, cellW, 2.8f, 0.15f, COL_BRICK_DARK * 0.7f, 4.0f);

                // Cell door (heavy wood, facing corridor)
                shader.setVec3("material.diffuse", COL_WOOD_DARK * 0.4f);
                float doorZ = (side == 0) ? cellZ + cellD : cellZ - 0.12f;
                cube.draw(shader, I, cellX + 0.15f, 0.0f, doorZ, cellW - 0.3f, 2.0f, 0.12f, COL_WOOD_DARK * 0.5f, 4.0f);
            }
        }
        unbind(shader);
    }

    // ================================================================
    // Window glow (emissive quads, visible from 25-50m)
    // ================================================================
    void renderWindowGlow(Shader& shader, const glm::mat4& I,
                          Cube& cube, int blockNum) const
    {
        glm::vec3 sw = BarrackGrid::getBlockSW(blockNum);
        float winW = 1.0f, winH = 1.2f;
        float firstWinX = 2.5f, winSpacingX = (BarrackGrid::BLOCK_LEN - 5.0f) / 6.0f;

        shader.setBool("useTexture", false);
        shader.setVec3("material.ambient", glm::vec3(0.0f));
        shader.setVec3("material.diffuse", glm::vec3(0.0f));
        shader.setVec3("material.specular", glm::vec3(0.0f));
        shader.setFloat("material.shininess", 1.0f);
        shader.setVec3("material.emissive", glm::vec3(0.6f, 0.55f, 0.3f)); // warm yellow glow

        for (int storey = 0; storey < 2; storey++) {
            float winY = (storey == 0) ? 1.4f : 3.9f;
            for (int w = 0; w < 7; w++) {
                float wx = sw.x + firstWinX + w * winSpacingX - winW * 0.5f;
                // South face glow (slightly inside the window cutout)
                cube.draw(shader, I, wx, winY, sw.z - 0.05f, winW, winH, 0.02f, glm::vec3(0.6f, 0.55f, 0.3f), 1.0f);
                // North face glow
                cube.draw(shader, I, wx, winY, sw.z + BarrackGrid::BLOCK_WID + 0.03f, winW, winH, 0.02f, glm::vec3(0.6f, 0.55f, 0.3f), 1.0f);
            }
        }

        shader.setVec3("material.emissive", glm::vec3(0.0f)); // reset
    }
};

#endif
