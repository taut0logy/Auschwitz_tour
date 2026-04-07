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
// ================================================================
class GroundZone {
public:
    void render(Shader& shader, const glm::mat4& I,
                Cube& cube, Plane& plane,
                unsigned int texGravel, unsigned int texDirtRoad,
                unsigned int texCobblestone, unsigned int texConcrete) const
    {
        // ---- MAIN GROUND PLANE ----
        // Camp interior spans ~270x210 m, but we extend further for the exterior
        bindTex(shader, texGravel, 50.0f);
        setMat(shader, 0.22f, 0.92f, 0.02f, 2.0f);
        // Main ground from fence perimeter outward slightly
        plane.draw(shader, I, -140.0f, 0.0f, -100.0f, 280.0f, 1.0f, 200.0f, COL_GRAVEL, 2.0f);
        unbind(shader);

        // ---- LAGERSTRASSE (main E-W road) ----
        // Z = -68, width 7 m, from X = -130 to X = +130
        bindTex(shader, texDirtRoad, 20.0f);
        setMat(shader, 0.20f, 0.88f, 0.01f, 2.0f);
        plane.draw(shader, I, -130.0f, 0.02f, -71.5f, 260.0f, 1.0f, 7.0f, COL_DIRT_ROAD, 4.0f);

        // ---- INTER-ROW ROADS (N-S, between each row pair) ----
        // Row Z-centres: -54, -24, 6, 36
        plane.draw(shader, I, -130.0f, 0.02f, -44.0f, 260.0f, 1.0f, 10.0f, COL_DIRT_ROAD, 4.0f);
        plane.draw(shader, I, -130.0f, 0.02f, -14.0f, 260.0f, 1.0f, 10.0f, COL_DIRT_ROAD, 4.0f);
        plane.draw(shader, I, -130.0f, 0.02f, 16.0f, 260.0f, 1.0f, 10.0f, COL_DIRT_ROAD, 4.0f);
        // South verge road
        plane.draw(shader, I, -130.0f, 0.02f, 44.0f, 260.0f, 1.0f, 6.0f, COL_DIRT_ROAD, 4.0f);

        // ---- INTER-COLUMN ROADS (E-W, between column pairs) ----
        // Column X-centres: -116, -78, -40, -2, 36, 74, 112
        // Gaps between columns
        float colGaps[] = { -97.0f, -59.0f, -21.0f, 17.0f, 55.0f, 93.0f };
        for (int i = 0; i < 6; i++) {
            float rx = colGaps[i] - 4.0f; // road 8m wide, centred at gap
            plane.draw(shader, I, rx, 0.02f, -65.0f, 8.0f, 1.0f, 115.0f, COL_DIRT_ROAD, 4.0f);
        }
        unbind(shader);

        // ---- APPELLPLATZ (Roll-Call Square) ----
        // Centred at X=+36, Z=-13, dimensions 55m x 90m
        bindTex(shader, texGravel, 30.0f);
        setMat(shader, 0.22f, 0.92f, 0.02f, 2.0f);
        plane.draw(shader, I, 8.5f, 0.03f, -58.0f, 55.0f, 1.0f, 90.0f,
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
