#ifndef GROUNDZONE_H
#define GROUNDZONE_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Shader.h"
#include "primitives/Cube.h"
#include "primitives/Plane.h"

// ================================================================
// GroundZone: Ground plane, road network, Appellplatz
// Per Section 2.2 of the v2 specification
//
// UPDATED: Road positions synchronized with new barracks layout
// Gate moved to X=+155, camp spans X=-132 to X=+132
// ================================================================
class GroundZone {
public:
    void render(Shader& shader, const glm::mat4& I,
                Cube& cube, Plane& plane,
                unsigned int texGravel, unsigned int texDirtRoad,
                unsigned int texCobblestone, unsigned int texConcrete) const
    {
        // ---- MAIN GROUND PLANE ----
        // Extended to cover gate area at X=+155
        bindTex(shader, texGravel, 50.0f);
        setMat(shader, 0.22f, 0.92f, 0.02f, 2.0f);
        // Main ground from X=-140 to X=+180 (includes gate area)
        plane.draw(shader, I, -140.0f, 0.0f, -100.0f, 320.0f, 1.0f, 200.0f, COL_GRAVEL, 2.0f);
        unbind(shader);

        // ---- LAGERSTRASSE (main E-W road) ----
        // North of camp, Z = -72 (just north of northernmost row at Z=-54)
        // Extended to reach gate at X=+155
        bindTex(shader, texDirtRoad, 20.0f);
        setMat(shader, 0.20f, 0.88f, 0.01f, 2.0f);
        // Extended from X=-130 to X=+160 to reach gate
        plane.draw(shader, I, -130.0f, 0.02f, -75.5f, 290.0f, 1.0f, 7.0f, COL_DIRT_ROAD, 4.0f);

        // ---- INTER-ROW ROADS (N-S, between each row pair) ----
        // Row Z-centres: -54, -18, +18, +54
        // Road positions: halfway between rows, 10m width
        // Between -54 and -18: Z = -36
        // Between -18 and +18: Z = 0  
        // Between +18 and +54: Z = +36
        // South of +54: Z = +70
        plane.draw(shader, I, -130.0f, 0.02f, -41.0f, 290.0f, 1.0f, 10.0f, COL_DIRT_ROAD, 4.0f);
        plane.draw(shader, I, -130.0f, 0.02f, 0.0f, 290.0f, 1.0f, 10.0f, COL_DIRT_ROAD, 4.0f);
        plane.draw(shader, I, -130.0f, 0.02f, +36.0f, 290.0f, 1.0f, 10.0f, COL_DIRT_ROAD, 4.0f);
        // South verge road
        plane.draw(shader, I, -130.0f, 0.02f, +70.0f, 290.0f, 1.0f, 6.0f, COL_DIRT_ROAD, 4.0f);

        // ---- INTER-COLUMN ROADS (E-W, between column pairs) ----
        // Column X-centres: -112, -64, -16, +32, +80, +128
        // Roads at gaps: between -112/-64, -64/-16, -16/+32, +32/+80, +80/+128
        // Gap sizes: 48m everywhere
        float colGaps[] = { -88.0f, -40.0f, +8.0f, +56.0f, +104.0f };
        for (int i = 0; i < 5; i++) {
            float rx = colGaps[i] - 4.0f; // road 8m wide, centred at gap
            // Extended Z range to cover all 4 rows with 12m width each
            // Z from -72 (south of -54) to +72 (north of +54)
            plane.draw(shader, I, rx, 0.02f, -72.0f, 8.0f, 1.0f, 144.0f, COL_DIRT_ROAD, 4.0f);
        }
        unbind(shader);

        // ---- GATE APPROACH ROAD ----
        // Road from camp to gate at X=+155
        bindTex(shader, texDirtRoad, 15.0f);
        setMat(shader, 0.20f, 0.88f, 0.01f, 2.0f);
        // From camp edge (X=+132) to gate (X=+155), 4m wide, centered at Z=0
        plane.draw(shader, I, +132.0f, 0.02f, -2.0f, 23.0f, 1.0f, 4.0f, COL_DIRT_ROAD, 4.0f);
        unbind(shader);

        // ---- APPELLPLATZ (Roll-Call Square / central courtyard) ----
        // Centered in camp core at X=+8, Z=0.
        // Dimensions tuned to remain clear of main circulation strips.
        bindTex(shader, texGravel, 30.0f);
        setMat(shader, 0.22f, 0.92f, 0.02f, 2.0f);
        // X centred at +8: from -16 to +32
        // Z centred at 0: from -41 to +41
        plane.draw(shader, I, -16.0f, 0.03f, -41.0f, 48.0f, 1.0f, 82.0f,
                   COL_GRAVEL * 0.85f, 2.0f);
        unbind(shader);
    }

private:
    static inline const glm::vec3 COL_GRAVEL    = glm::vec3(0.40f, 0.38f, 0.35f);
    static inline const glm::vec3 COL_DIRT_ROAD = glm::vec3(0.45f, 0.38f, 0.28f);

    void bindTex(Shader& s, unsigned int tex, float rep) const {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex);
        s.setInt("texture1", 0);
        s.setBool("useTexture", tex != 0);
        s.setFloat("texRepeat", rep);
    }
    void unbind(Shader& s) const { s.setBool("useTexture", false); }
    void setMat(Shader& s, float a, float d, float sp, float sh) const {
        s.setVec3("material.ambient", COL_GRAVEL * a);
        s.setVec3("material.diffuse", COL_GRAVEL * d);
        s.setVec3("material.specular", glm::vec3(sp));
        s.setFloat("material.shininess", sh);
        s.setVec3("material.emissive", glm::vec3(0.0f));
    }
};

#endif
