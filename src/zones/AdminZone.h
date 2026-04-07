#ifndef ADMINZONE_H
#define ADMINZONE_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Shader.h"
#include "primitives/Cube.h"

// ================================================================
// AdminZone: 4 administrative buildings north of barracks grid
// Per Section 2.7
// Located roughly at X = +20 to +90, Z = -80 to -68
// ================================================================
class AdminZone {
public:
    void render(Shader& shader, const glm::mat4& I,
                Cube& cube,
                unsigned int texBrickRed, unsigned int texRoofTile) const
    {
        bindTex(shader, texBrickRed, 6.0f);
        setMaterial(shader, COL_BRICK_RED, 0.12f, 0.82f, 0.04f, 4.0f);

        // 1. Commandant's office: X=+65, Z=-80, 20×10×6m, two storeys
        float b1x = 65.0f - 10.0f, b1z = -80.0f - 5.0f;
        cube.draw(shader, I, b1x, 0.0f, b1z, 20.0f, 6.0f, 10.0f, COL_BRICK_RED * 1.05f, 4.0f);

        // 2. SS administration: X=+30, Z=-80, 24×12×6m
        float b2x = 30.0f - 12.0f, b2z = -80.0f - 6.0f;
        cube.draw(shader, I, b2x, 0.0f, b2z, 24.0f, 6.0f, 12.0f, COL_BRICK_RED, 4.0f);

        // 3. Kitchen: X=-10, Z=-80, 22×10×5m, single storey
        float b3x = -10.0f - 11.0f, b3z = -80.0f - 5.0f;
        cube.draw(shader, I, b3x, 0.0f, b3z, 22.0f, 5.0f, 10.0f, COL_BRICK_RED * 0.95f, 4.0f);

        // 4. Reception/bathhouse: X=+90, Z=-80, 18×12×5.5m
        float b4x = 90.0f - 9.0f, b4z = -80.0f - 6.0f;
        cube.draw(shader, I, b4x, 0.0f, b4z, 18.0f, 5.5f, 12.0f, COL_BRICK_RED * 0.95f, 4.0f);
        unbind(shader);

        // ---- Roofs (gabled approximation) ----
        bindTex(shader, texRoofTile, 6.0f);
        setMaterial(shader, COL_ROOF_TILE, 0.13f, 0.72f, 0.06f, 8.0f);

        // Commandant's office roof
        cube.draw(shader, I, b1x - 0.5f, 6.0f, b1z - 0.5f, 21.0f, 0.3f, 11.0f, COL_ROOF_TILE, 8.0f);
        cube.draw(shader, I, b1x + 1.0f, 6.3f, b1z + 0.5f, 18.0f, 0.3f, 9.0f, COL_ROOF_TILE, 8.0f);
        cube.draw(shader, I, b1x + 2.5f, 6.6f, b1z + 1.5f, 15.0f, 0.25f, 7.0f, COL_ROOF_TILE, 8.0f);

        // SS admin roof
        cube.draw(shader, I, b2x - 0.5f, 6.0f, b2z - 0.5f, 25.0f, 0.3f, 13.0f, COL_ROOF_TILE, 8.0f);
        cube.draw(shader, I, b2x + 1.0f, 6.3f, b2z + 0.5f, 22.0f, 0.3f, 11.0f, COL_ROOF_TILE, 8.0f);
        cube.draw(shader, I, b2x + 2.5f, 6.6f, b2z + 1.5f, 19.0f, 0.25f, 9.0f, COL_ROOF_TILE, 8.0f);

        // Kitchen roof
        cube.draw(shader, I, b3x - 0.5f, 5.0f, b3z - 0.5f, 23.0f, 0.3f, 11.0f, COL_ROOF_TILE, 8.0f);
        cube.draw(shader, I, b3x + 1.0f, 5.3f, b3z + 0.5f, 20.0f, 0.25f, 9.0f, COL_ROOF_TILE, 8.0f);

        // Reception roof
        cube.draw(shader, I, b4x - 0.5f, 5.5f, b4z - 0.5f, 19.0f, 0.3f, 13.0f, COL_ROOF_TILE, 8.0f);
        cube.draw(shader, I, b4x + 1.0f, 5.8f, b4z + 0.5f, 16.0f, 0.25f, 11.0f, COL_ROOF_TILE, 8.0f);
        unbind(shader);

        // ---- Windows on admin buildings ----
        shader.setBool("useTexture", false);
        shader.setVec3("material.ambient", glm::vec3(0.02f));
        shader.setVec3("material.diffuse", glm::vec3(0.05f));
        shader.setVec3("material.specular", glm::vec3(0.3f));
        shader.setFloat("material.shininess", 64.0f);

        // Commandant: windows on south face
        for (int w = 0; w < 6; w++) {
            float wx = b1x + 2.0f + w * 3.0f;
            for (int s = 0; s < 2; s++) {
                float wy = 1.5f + s * 3.0f;
                cube.draw(shader, I, wx, wy, b1z - 0.02f, 1.0f, 1.2f, 0.04f, glm::vec3(0.1f, 0.12f, 0.15f), 64.0f);
            }
        }

        // SS admin: windows on south face
        for (int w = 0; w < 7; w++) {
            float wx = b2x + 2.0f + w * 3.0f;
            for (int s = 0; s < 2; s++) {
                float wy = 1.5f + s * 3.0f;
                cube.draw(shader, I, wx, wy, b2z - 0.02f, 1.0f, 1.2f, 0.04f, glm::vec3(0.1f, 0.12f, 0.15f), 64.0f);
            }
        }
    }

private:
    static inline const glm::vec3 COL_BRICK_RED  = glm::vec3(0.55f, 0.27f, 0.07f);
    static inline const glm::vec3 COL_ROOF_TILE  = glm::vec3(0.35f, 0.30f, 0.25f);

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
