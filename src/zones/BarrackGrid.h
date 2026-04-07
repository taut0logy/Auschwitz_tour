#ifndef BARRACKGRID_H
#define BARRACKGRID_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <cmath>
#include "Shader.h"
#include "primitives/Cube.h"
#include "primitives/Cylinder.h"
#include "primitives/Plane.h"

// ================================================================
// BarrackGrid: 28 barrack blocks — full exterior geometry
// Per Section 2.1 and 3.1 of the v2 specification
//
// Each block: 40m long (X) × 12m wide (Z) × 8m tall + 2.5m gabled roof
// 4 rows × 7 columns
// ================================================================
class BarrackGrid {
public:
    // Column X-centres (7 columns)
    static constexpr float COL_X[7] = { -116.0f, -78.0f, -40.0f, -2.0f, 36.0f, 74.0f, 112.0f };
    // Row Z-centres (4 rows)
    static constexpr float ROW_Z[4] = { -54.0f, -24.0f, 6.0f, 36.0f };

    static constexpr float BLOCK_LEN   = 32.0f;   // X (length spanning east-west)
    static constexpr float BLOCK_WID   = 10.0f;   // Z (width spanning north-south)
    static constexpr float BLOCK_HT    = 6.0f;    // wall height
    static constexpr float ROOF_RISE   = 2.5f;    // gable ridge above walls
    static constexpr float ROOF_THICK  = 0.3f;

    void render(Shader& shader, const glm::mat4& I,
                Cube& cube, Cylinder& cyl, Plane& plane,
                unsigned int texBrickRed, unsigned int texBrickDark,
                unsigned int texRoofTile, unsigned int texConcrete,
                unsigned int texWoodPlank, unsigned int texWoodDark,
                unsigned int texGlassAlpha) const
    {
        for (int row = 0; row < 4; row++) {
            for (int col = 0; col < 7; col++) {
                int blockNum = row * 7 + col + 1;
                float cx = COL_X[col];    // block centre X
                float cz = ROW_Z[row];    // block centre Z

                // Block southwest corner (origin for local coords)
                float bx = cx - BLOCK_LEN * 0.5f;
                float bz = cz - BLOCK_WID * 0.5f;

                bool isDark = (blockNum == 10 || blockNum == 11);

                renderBlockExterior(shader, I, cube, cyl, plane,
                    bx, bz, isDark, blockNum,
                    texBrickRed, texBrickDark, texRoofTile,
                    texConcrete, texWoodPlank, texWoodDark, texGlassAlpha);
            }
        }
    }

    // Get block centre position for a given block number (1-28)
    static glm::vec3 getBlockCentre(int blockNum) {
        int idx = blockNum - 1;
        int row = idx / 7;
        int col = idx % 7;
        return glm::vec3(COL_X[col], 0.0f, ROW_Z[row]);
    }

    // Get block southwest corner for a given block number
    static glm::vec3 getBlockSW(int blockNum) {
        glm::vec3 c = getBlockCentre(blockNum);
        return glm::vec3(c.x - BLOCK_LEN * 0.5f, 0.0f, c.z - BLOCK_WID * 0.5f);
    }

private:
    static inline const glm::vec3 COL_BRICK_RED   = glm::vec3(0.55f, 0.27f, 0.07f);
    static inline const glm::vec3 COL_BRICK_DARK  = glm::vec3(0.35f, 0.17f, 0.05f);
    static inline const glm::vec3 COL_ROOF_TILE   = glm::vec3(0.35f, 0.30f, 0.25f);
    static inline const glm::vec3 COL_WOOD        = glm::vec3(0.45f, 0.30f, 0.15f);
    static inline const glm::vec3 COL_WOOD_DARK   = glm::vec3(0.30f, 0.18f, 0.08f);
    static inline const glm::vec3 COL_CONCRETE    = glm::vec3(0.60f, 0.58f, 0.55f);

    void bindTex(Shader& s, unsigned int tex, float rep) const {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex);
        s.setInt("texture1", 0);
        s.setBool("useTexture", tex != 0);
        s.setFloat("texRepeat", rep);
    }
    void unbind(Shader& s) const { s.setBool("useTexture", false); }

    void renderBlockExterior(Shader& shader, const glm::mat4& I,
        Cube& cube, Cylinder& cyl, Plane& plane,
        float bx, float bz, bool isDark, int blockNum,
        unsigned int texBrickRed, unsigned int texBrickDark, unsigned int texRoofTile,
        unsigned int texConcrete, unsigned int texWoodPlank, unsigned int texWoodDark,
        unsigned int texGlassAlpha) const
    {
        glm::vec3 wallCol = isDark ? COL_BRICK_DARK : COL_BRICK_RED;
        unsigned int wallTex = isDark ? texBrickDark : texBrickRed;

        // =============================================
        // 1. Foundation ledge (below ground level)
        // 40.4 × 0.4 × 12.4, at Y = -0.4 to 0.0
        // =============================================
        bindTex(shader, texConcrete, 8.0f);
        shader.setVec3("material.ambient", COL_CONCRETE * 0.18f);
        shader.setVec3("material.diffuse", COL_CONCRETE * 0.82f);
        shader.setVec3("material.specular", glm::vec3(0.06f));
        shader.setFloat("material.shininess", 6.0f);
        shader.setVec3("material.emissive", glm::vec3(0.0f));
        cube.draw(shader, I, bx - 0.2f, -0.4f, bz - 0.2f,
                  BLOCK_LEN + 0.4f, 0.4f, BLOCK_WID + 0.4f, COL_CONCRETE, 6.0f);

        // =============================================
        // 2. Main wall box (40 × 8 × 12)
        // UV: repeat every 0.5m → 80 repeats along 40m
        // =============================================
        bindTex(shader, wallTex, 80.0f);
        shader.setVec3("material.ambient", wallCol * (isDark ? 0.06f : 0.12f));
        shader.setVec3("material.diffuse", wallCol * (isDark ? 0.65f : 0.82f));
        shader.setVec3("material.specular", glm::vec3(isDark ? 0.03f : 0.04f));
        shader.setFloat("material.shininess", 4.0f);
        shader.setVec3("material.emissive", glm::vec3(0.0f));
        cube.draw(shader, I, bx, 0.0f, bz, BLOCK_LEN, BLOCK_HT, BLOCK_WID, wallCol, 4.0f);

        // =============================================
        // 3. Gabled roof
        // Two sloped panels meeting at ridge Y = BLOCK_HT + ROOF_RISE
        // Overhang: 0.5m on long sides, 0.3m on gable ends
        // =============================================
        bindTex(shader, texRoofTile, 40.0f);
        shader.setVec3("material.ambient", COL_ROOF_TILE * 0.13f);
        shader.setVec3("material.diffuse", COL_ROOF_TILE * 0.72f);
        shader.setVec3("material.specular", glm::vec3(0.06f));
        shader.setFloat("material.shininess", 8.0f);

        float ridgeY = BLOCK_HT + ROOF_RISE;
        float slopeAngle = atanf(ROOF_RISE / (BLOCK_WID * 0.5f));
        float slopeLen = sqrtf((BLOCK_WID * 0.5f + 0.5f) * (BLOCK_WID * 0.5f + 0.5f) + ROOF_RISE * ROOF_RISE);

        // Left roof slope (South side, starts at south edge, draws up toward ridge)
        glm::mat4 roofL = glm::translate(I, glm::vec3(bx - 0.3f, BLOCK_HT, bz - 0.5f));
        roofL = glm::rotate(roofL, slopeAngle, glm::vec3(1.0f, 0.0f, 0.0f));
        cube.draw(shader, roofL, 0, 0, 0, BLOCK_LEN + 0.6f, ROOF_THICK, slopeLen, COL_ROOF_TILE, 8.0f);

        // Right roof slope (North side, starts at north edge, draws up toward ridge backwards)
        glm::mat4 roofR = glm::translate(I, glm::vec3(bx - 0.3f, BLOCK_HT, bz + BLOCK_WID + 0.5f));
        roofR = glm::rotate(roofR, -slopeAngle, glm::vec3(1.0f, 0.0f, 0.0f));
        // Drawing into negative Z pushes it Southwards back to the ridge
        cube.draw(shader, roofR, 0, -ROOF_THICK, -slopeLen, BLOCK_LEN + 0.6f, ROOF_THICK, slopeLen, COL_ROOF_TILE, 8.0f);

        // Ridge beam
        cube.draw(shader, I, bx - 0.3f, ridgeY - 0.125f, bz + BLOCK_WID * 0.5f - 0.125f,
                  BLOCK_LEN + 0.6f, 0.25f, 0.25f, COL_ROOF_TILE * 0.8f, 4.0f);
        unbind(shader);

        // =============================================
        // 4. Windows (7 per storey per long face = 28 total)
        // Each window: 1.0m wide × 1.2m tall, recessed 0.15m
        // Frame: 0.08m wide box strips
        // =============================================
        bindTex(shader, texWoodDark, 2.0f);
        shader.setVec3("material.ambient", COL_WOOD_DARK * 0.15f);
        shader.setVec3("material.diffuse", COL_WOOD_DARK * 0.70f);
        shader.setVec3("material.specular", glm::vec3(0.05f));
        shader.setFloat("material.shininess", 6.0f);
        shader.setVec3("material.emissive", glm::vec3(0.0f));

        float winW = 1.0f, winH = 1.2f;
        float frameW = 0.08f;
        float firstWinX = 2.5f;
        float winSpacingX = (BLOCK_LEN - 5.0f) / 6.0f; 

        for (int storey = 0; storey < 2; storey++) {
            float winY = (storey == 0) ? 2.0f - winH * 0.5f : 4.5f - winH * 0.5f;

            for (int w = 0; w < 7; w++) {
                float winLocalX = firstWinX + w * winSpacingX;

                // South face windows (Z = bz)
                // Top frame
                cube.draw(shader, I, bx + winLocalX - frameW, winY + winH, bz - 0.04f,
                          winW + frameW * 2, frameW, 0.15f, COL_WOOD_DARK, 6.0f);
                // Bottom frame (sill)
                cube.draw(shader, I, bx + winLocalX - frameW, winY - frameW, bz - 0.06f,
                          winW + frameW * 2, frameW, 0.18f, COL_WOOD_DARK, 6.0f);
                // Left frame
                cube.draw(shader, I, bx + winLocalX - frameW, winY, bz - 0.04f,
                          frameW, winH, 0.15f, COL_WOOD_DARK, 6.0f);
                // Right frame
                cube.draw(shader, I, bx + winLocalX + winW, winY, bz - 0.04f,
                          frameW, winH, 0.15f, COL_WOOD_DARK, 6.0f);

                // North face windows (Z = bz + BLOCK_WID)
                cube.draw(shader, I, bx + winLocalX - frameW, winY + winH, bz + BLOCK_WID - 0.11f,
                          winW + frameW * 2, frameW, 0.15f, COL_WOOD_DARK, 6.0f);
                cube.draw(shader, I, bx + winLocalX - frameW, winY - frameW, bz + BLOCK_WID - 0.12f,
                          winW + frameW * 2, frameW, 0.18f, COL_WOOD_DARK, 6.0f);
                cube.draw(shader, I, bx + winLocalX - frameW, winY, bz + BLOCK_WID - 0.11f,
                          frameW, winH, 0.15f, COL_WOOD_DARK, 6.0f);
                cube.draw(shader, I, bx + winLocalX + winW, winY, bz + BLOCK_WID - 0.11f,
                          frameW, winH, 0.15f, COL_WOOD_DARK, 6.0f);

                // Iron bars for Block 11 / Block 10
                if (isDark) {
                    for (int bar = 0; bar < 6; bar++) {
                        float barX = bx + winLocalX + (float)(bar + 1) * winW / 7.0f;
                        // South face bars
                        cyl.draw(shader, I, barX - 0.015f, winY, bz - 0.05f,
                                 0.03f, winH, 0.03f, glm::vec3(0.3f), 32.0f);
                        // North face bars
                        cyl.draw(shader, I, barX - 0.015f, winY, bz + BLOCK_WID + 0.02f,
                                 0.03f, winH, 0.03f, glm::vec3(0.3f), 32.0f);
                    }
                }
            }
        }

        // Gable end windows (2 per gable, at east and west walls)
        for (int side = 0; side < 2; side++) {
            float gableX = (side == 0) ? bx - 0.04f : bx + BLOCK_LEN - 0.11f;
            for (int storey = 0; storey < 2; storey++) {
                float winY = (storey == 0) ? 2.0f - winH * 0.5f : 4.5f - winH * 0.5f;
                float winLocalZ = BLOCK_WID * 0.5f - winW * 0.5f;
                // Frame on gable end
                cube.draw(shader, I, gableX, winY + winH, bz + winLocalZ - frameW,
                          0.15f, frameW, winW + frameW * 2, COL_WOOD_DARK, 6.0f);
                cube.draw(shader, I, gableX, winY - frameW, bz + winLocalZ - frameW,
                          0.15f, frameW, winW + frameW * 2, COL_WOOD_DARK, 6.0f);
                cube.draw(shader, I, gableX, winY, bz + winLocalZ - frameW,
                          0.15f, winH, frameW, COL_WOOD_DARK, 6.0f);
                cube.draw(shader, I, gableX, winY, bz + winLocalZ + winW,
                          0.15f, winH, frameW, COL_WOOD_DARK, 6.0f);
            }
        }
        unbind(shader);

        // =============================================
        // 5. Doors (1 per gable end, ground floor)
        // 1.2m wide × 2.2m tall, centred on wall
        // =============================================
        bindTex(shader, texWoodDark, 2.0f);
        float doorW = 1.2f, doorH = 2.2f;
        float doorZ = bz + BLOCK_WID * 0.5f - doorW * 0.5f;

        // West gable door
        cube.draw(shader, I, bx - 0.12f, 0.0f, doorZ, 0.12f, doorH, doorW, COL_WOOD_DARK, 6.0f);
        // East gable door
        cube.draw(shader, I, bx + BLOCK_LEN, 0.0f, doorZ, 0.12f, doorH, doorW, COL_WOOD_DARK, 6.0f);

        // Door frames
        cube.draw(shader, I, bx - 0.15f, doorH, doorZ - 0.1f, 0.15f, 0.1f, doorW + 0.2f, COL_WOOD_DARK, 6.0f);
        cube.draw(shader, I, bx + BLOCK_LEN, doorH, doorZ - 0.1f, 0.15f, 0.1f, doorW + 0.2f, COL_WOOD_DARK, 6.0f);
        unbind(shader);

        // =============================================
        // 6. Chimney stack (on roof ridge)
        // 0.6 × 0.6 × 1.8m above ridge, brick_dark
        // Located at X_local = 4m, Z_local = 6m (on ridge)
        // =============================================
        bindTex(shader, texBrickDark, 3.0f);
        shader.setVec3("material.ambient", COL_BRICK_DARK * 0.10f);
        shader.setVec3("material.diffuse", COL_BRICK_DARK * 0.70f);
        shader.setVec3("material.specular", glm::vec3(0.03f));
        shader.setFloat("material.shininess", 4.0f);
        shader.setVec3("material.emissive", glm::vec3(0.0f));
        cube.draw(shader, I, bx + 4.0f - 0.3f, ridgeY, bz + 6.0f - 0.3f,
                  0.6f, 1.8f, 0.6f, COL_BRICK_DARK, 4.0f);
        unbind(shader);
    }
};

#endif
