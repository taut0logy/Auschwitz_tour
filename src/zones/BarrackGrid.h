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
//
// REPOSITIONED: Shifted west to accommodate gate at X=+155
// Updated to match prompt_v2 spec dimensions exactly
// ================================================================
class BarrackGrid {
public:
    // FIXED: Column X-centres with PROPER SPACING
    // Block length = 40m, with 8m gap between blocks
    // Spacing = 48m between column centers
    // Col 0 at -112 spans -132 to -92
    // Col 1 at -64 spans -84 to -44 (8m gap from col 0)
    // Col 6 at +128 spans +108 to +148, gate at +155 leaves 7m clearance
    static constexpr float COL_X[6] = { -112.0f, -64.0f, -16.0f, +32.0f, +80.0f, +128.0f };
    // Row Z-centres with proper spacing for 12m wide blocks + 24m gaps
    static constexpr float ROW_Z[4] = { -54.0f, -18.0f, +18.0f, +54.0f };

    // Updated to match prompt_v2 specification exactly
    static constexpr float BLOCK_LEN   = 40.0f;   // X (length spanning east-west) - was 32m
    static constexpr float BLOCK_WID   = 12.0f;   // Z (width spanning north-south) - was 10m
    static constexpr float BLOCK_HT    = 8.0f;    // wall height - was 6m
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
            for (int col = 0; col < 6; col++) {
                int blockNum = row * 6 + col + 1;
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

    // Get block centre position for a given block number (1-24)
    static glm::vec3 getBlockCentre(int blockNum) {
        int idx = blockNum - 1;
        int row = idx / 6;
        int col = idx % 6;
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
        // 2. Main walls (with cutouts for windows and doors)
        // 40 × 8 × 12, wall thickness = 0.2m
        // =============================================
        bindTex(shader, wallTex, 80.0f);
        shader.setVec3("material.ambient", wallCol * (isDark ? 0.06f : 0.12f));
        shader.setVec3("material.diffuse", wallCol * (isDark ? 0.65f : 0.82f));
        shader.setVec3("material.specular", glm::vec3(isDark ? 0.03f : 0.04f));
        shader.setFloat("material.shininess", 4.0f);
        shader.setVec3("material.emissive", glm::vec3(0.0f));

        float thick = 0.2f;
        auto drawStrip = [&](float x, float y, float z, float w, float h, float d) {
            cube.draw(shader, I, x, y, z, w, h, d, wallCol, 4.0f);
        };

        float winW = 1.0f, winH = 1.2f;
        float firstWinX = 2.5f;
        float winSpacingX = (BLOCK_LEN - 5.0f) / 6.0f;

        // South & North Walls
        for (int side = 0; side < 2; side++) {
            float wz = (side == 0) ? bz : bz + BLOCK_WID - thick;
            
            // Solid horizontal bands
            drawStrip(bx, 0.0f, wz, BLOCK_LEN, 1.4f, thick); // bottom
            drawStrip(bx, 2.6f, wz, BLOCK_LEN, 1.3f, thick); // middle
            drawStrip(bx, 5.1f, wz, BLOCK_LEN, 2.9f, thick); // top

            // Pillars between windows
            for (int storey = 0; storey < 2; storey++) {
                float wy = (storey == 0) ? 1.4f : 3.9f;
                float curX = 0.0f;
                for (int w = 0; w < 7; w++) {
                    float cx = firstWinX + w * winSpacingX;
                    float left = cx - winW * 0.5f;
                    drawStrip(bx + curX, wy, wz, left - curX, winH, thick);
                    curX = cx + winW * 0.5f;
                }
                drawStrip(bx + curX, wy, wz, BLOCK_LEN - curX, winH, thick);
            }
        }

        // West & East Walls (Gable ends)
        float innerZ = bz + thick;
        float innerLen = BLOCK_WID - 2.0f * thick;
        float doorW = 1.2f, doorH = 2.2f;
        float doorLeft = (BLOCK_WID * 0.5f) - (doorW * 0.5f) - thick;
        float gableWinLeft = (BLOCK_WID * 0.5f) - (winW * 0.5f) - thick;

        for (int side = 0; side < 2; side++) {
            float wx = (side == 0) ? bx : bx + BLOCK_LEN - thick;
            
            // Y=0 to 2.2 (Door gap)
            drawStrip(wx, 0.0f, innerZ, thick, doorH, doorLeft);
            drawStrip(wx, 0.0f, innerZ + doorLeft + doorW, thick, doorH, innerLen - doorLeft - doorW);
            
            // Y=2.2 to 3.9 (solid middle)
            drawStrip(wx, doorH, innerZ, thick, 3.9f - doorH, innerLen);

            // Y=3.9 to 5.1 (Storey 1 Window gap)
            drawStrip(wx, 3.9f, innerZ, thick, winH, gableWinLeft);
            drawStrip(wx, 3.9f, innerZ + gableWinLeft + winW, thick, winH, innerLen - gableWinLeft - winW);

            // Y=5.1 to 8.0 (solid top)
            drawStrip(wx, 5.1f, innerZ, thick, BLOCK_HT - 5.1f, innerLen);
        }

        // =============================================
        // 3. Gabled Roof - CORRECTED: Two sloped panels + gable walls
        // Roof runs along X (building length), slopes along Z (building width)
        // Panels rotated around X-axis, starting from wall top
        // =============================================
        
        float ridgeY = BLOCK_HT + ROOF_RISE;
        float overhangX = 0.5f;   // Gable end overhang
        float overhangZ = 0.5f;   // Side overhang
        float halfRun = BLOCK_WID * 0.5f + overhangZ;
        float slantLen = sqrtf(halfRun * halfRun + ROOF_RISE * ROOF_RISE);
        float slopeAngle = atan2f(ROOF_RISE, halfRun);
        float roofLenX = BLOCK_LEN + 2.0f * overhangX;
        
        // ---- Roof panels (wood/roof tile texture) ----
        bindTex(shader, texRoofTile, 40.0f);
        shader.setVec3("material.ambient", COL_ROOF_TILE * 0.13f);
        shader.setVec3("material.diffuse", COL_ROOF_TILE * 0.72f);
        shader.setVec3("material.specular", glm::vec3(0.06f));
        shader.setFloat("material.shininess", 8.0f);
        
        // South panel: starts at south edge, rotates UP toward ridge
        {
            glm::mat4 m = glm::translate(I, glm::vec3(bx - overhangX, BLOCK_HT, bz - overhangZ));
            m = glm::rotate(m, -slopeAngle, glm::vec3(1.0f, 0.0f, 0.0f));
            cube.draw(shader, m, 0.0f, 0.0f, 0.0f, roofLenX, ROOF_THICK, slantLen, COL_ROOF_TILE, 8.0f);
        }
        
        // North panel: starts at ridge, rotates DOWN toward north edge
        {
            glm::mat4 m = glm::translate(I, glm::vec3(bx - overhangX, BLOCK_HT + ROOF_RISE, bz + BLOCK_WID * 0.5f));
            m = glm::rotate(m, slopeAngle, glm::vec3(1.0f, 0.0f, 0.0f));
            cube.draw(shader, m, 0.0f, 0.0f, 0.0f, roofLenX, ROOF_THICK, slantLen, COL_ROOF_TILE, 8.0f);
            
            // Re-draw ridge cap here to seal the top cleanly
            cube.draw(shader, I, bx - overhangX, ridgeY - 0.125f, bz + BLOCK_WID * 0.5f - 0.125f,
                      roofLenX, 0.25f, 0.25f, COL_ROOF_TILE * 0.8f, 4.0f);
        }
        unbind(shader);
        
        // ---- Gable walls (triangular fill at east and west ends) ----
        // These fill the triangular gap under the roof at each end
        bindTex(shader, wallTex, 8.0f);  // Use same texture as walls
        shader.setVec3("material.ambient", wallCol * (isDark ? 0.06f : 0.12f));
        shader.setVec3("material.diffuse", wallCol * (isDark ? 0.65f : 0.82f));
        shader.setVec3("material.specular", glm::vec3(isDark ? 0.03f : 0.04f));
        shader.setFloat("material.shininess", 4.0f);
        
        const int steps = 40;  // Number of steps for smooth gable
        float sliceH = ROOF_RISE / (float)steps;
        
        for (int i = 0; i < steps; i++) {
            float baseY = BLOCK_HT + i * sliceH;
            float frac = (float)i / (float)steps;
            float sliceW = BLOCK_WID * (1.0f - frac);  // Width narrows toward ridge
            float sliceX = (BLOCK_WID - sliceW) * 0.5f;  // Centered
            
            // West gable (at bx)
            cube.draw(shader, I, bx, baseY, bz + sliceX, 0.15f, sliceH, sliceW, wallCol, 4.0f);
            
            // East gable (at bx + BLOCK_LEN)
            cube.draw(shader, I, bx + BLOCK_LEN - 0.15f, baseY, bz + sliceX, 0.15f, sliceH, sliceW, wallCol, 4.0f);
        }
        unbind(shader);

        // =============================================
        // 4. Windows (7 per storey per long face = 28 total)
        // Each window hole is 1.0m wide × 1.2m tall.
        // Wooden frames protrude 0.04m from both inside and outside of the 0.2m thick wall.
        // =============================================
        bindTex(shader, texWoodDark, 2.0f);
        shader.setVec3("material.ambient", COL_WOOD_DARK * 0.15f);
        shader.setVec3("material.diffuse", COL_WOOD_DARK * 0.70f);
        shader.setVec3("material.specular", glm::vec3(0.05f));
        shader.setFloat("material.shininess", 6.0f);
        shader.setVec3("material.emissive", glm::vec3(0.0f));

        float frameW = 0.08f;
        float frameD = thick + 0.08f; // 0.28m depth to protrude 0.04m on both sides
        float protrude = 0.04f;

        for (int storey = 0; storey < 2; storey++) {
            float winY = (storey == 0) ? 1.4f : 3.9f;

            for (int w = 0; w < 7; w++) {
                float winLocalX = firstWinX + w * winSpacingX - winW * 0.5f;

                // South face windows (Wall Z = bz to bz + thick)
                float sZ = bz - protrude;
                cube.draw(shader, I, bx + winLocalX - frameW, winY + winH, sZ, winW + frameW * 2, frameW, frameD, COL_WOOD_DARK, 6.0f); // Top
                cube.draw(shader, I, bx + winLocalX - frameW, winY - frameW, sZ, winW + frameW * 2, frameW, frameD + 0.02f, COL_WOOD_DARK, 6.0f); // Sill
                cube.draw(shader, I, bx + winLocalX - frameW, winY, sZ, frameW, winH, frameD, COL_WOOD_DARK, 6.0f); // Left
                cube.draw(shader, I, bx + winLocalX + winW, winY, sZ, frameW, winH, frameD, COL_WOOD_DARK, 6.0f); // Right

                // North face windows (Wall Z = bz + WID - thick to bz + WID)
                float nZ = bz + BLOCK_WID - thick - protrude;
                cube.draw(shader, I, bx + winLocalX - frameW, winY + winH, nZ, winW + frameW * 2, frameW, frameD, COL_WOOD_DARK, 6.0f);
                cube.draw(shader, I, bx + winLocalX - frameW, winY - frameW, nZ, winW + frameW * 2, frameW, frameD + 0.02f, COL_WOOD_DARK, 6.0f);
                cube.draw(shader, I, bx + winLocalX - frameW, winY, nZ, frameW, winH, frameD, COL_WOOD_DARK, 6.0f);
                cube.draw(shader, I, bx + winLocalX + winW, winY, nZ, frameW, winH, frameD, COL_WOOD_DARK, 6.0f);

                // Iron bars for Block 11 / Block 10
                if (isDark) {
                    for (int bar = 0; bar < 6; bar++) {
                        float barX = bx + winLocalX + (float)(bar + 1) * winW / 7.0f;
                        cyl.draw(shader, I, barX - 0.015f, winY, sZ + frameD * 0.5f, 0.03f, winH, 0.03f, glm::vec3(0.3f), 32.0f);
                        cyl.draw(shader, I, barX - 0.015f, winY, nZ + frameD * 0.5f, 0.03f, winH, 0.03f, glm::vec3(0.3f), 32.0f);
                    }
                }
            }
        }

        // Gable end windows (Only storey 1, to prevent clash with ground floor door)
        for (int side = 0; side < 2; side++) {
            float gableX = (side == 0) ? bx - protrude : bx + BLOCK_LEN - thick - protrude;
            float winY = 3.9f;
            float winLocalZ = BLOCK_WID * 0.5f - winW * 0.5f;

            cube.draw(shader, I, gableX, winY + winH, bz + winLocalZ - frameW, frameD, frameW, winW + frameW * 2, COL_WOOD_DARK, 6.0f);
            cube.draw(shader, I, gableX, winY - frameW, bz + winLocalZ - frameW, frameD + 0.02f, frameW, winW + frameW * 2, COL_WOOD_DARK, 6.0f);
            cube.draw(shader, I, gableX, winY, bz + winLocalZ - frameW, frameD, winH, frameW, COL_WOOD_DARK, 6.0f);
            cube.draw(shader, I, gableX, winY, bz + winLocalZ + winW, frameD, winH, frameW, COL_WOOD_DARK, 6.0f);
        }
        unbind(shader);

        // =============================================
        // 5. Doors (1 per gable end, ground floor)
        // 1.2m wide × 2.2m tall, centred on wall
        // =============================================
        bindTex(shader, texWoodDark, 2.0f);
        float doorZ = bz + BLOCK_WID * 0.5f - doorW * 0.5f;

        // West gable door
        cube.draw(shader, I, bx - protrude, 0.0f, doorZ, frameD, doorH, doorW, COL_WOOD_DARK, 6.0f);
        // East gable door
        cube.draw(shader, I, bx + BLOCK_LEN - thick - protrude, 0.0f, doorZ, frameD, doorH, doorW, COL_WOOD_DARK, 6.0f);

        // Door frames
        cube.draw(shader, I, bx - protrude - 0.02f, doorH, doorZ - 0.1f, frameD + 0.04f, 0.1f, doorW + 0.2f, COL_WOOD_DARK, 6.0f);
        cube.draw(shader, I, bx + BLOCK_LEN - thick - protrude - 0.02f, doorH, doorZ - 0.1f, frameD + 0.04f, 0.1f, doorW + 0.2f, COL_WOOD_DARK, 6.0f);
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
