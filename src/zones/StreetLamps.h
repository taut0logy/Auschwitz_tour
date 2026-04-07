#ifndef STREETLAMPS_H
#define STREETLAMPS_H

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
#include "primitives/Sphere.h"

// ================================================================
// StreetLamps: ~85 street lamp instances with Bezier curved arm,
// truncated cone housing, emissive bulb sphere, spotlight
// Per Sections 2.5, 3.4
// ================================================================

struct LampInstance {
    glm::vec3 position;
    float rotationY; // orient arm toward road
};

class StreetLamps {
public:
    MeshFlyweight armMesh;
    std::vector<LampInstance> lamps;

    void init() {
        buildArmMesh();
        placeLamps();
    }

    void render(Shader& shader, const glm::mat4& I,
                Cube& cube, Cylinder& cyl, Sphere& sphere,
                unsigned int texMetalGrey) const
    {
        for (const auto& lamp : lamps) {
            renderLamp(shader, I, cube, cyl, sphere, texMetalGrey, lamp);
        }
    }

    // Render lamp bulbs with day/night color behavior.
    void renderBulbs(Shader& unlitShader, const glm::mat4& I, Sphere& sphere,
                     bool isDaytime, float lampIntensity) const {
        glm::vec3 bulbColor = isDaytime
            ? glm::vec3(0.33f, 0.33f, 0.33f)
            : glm::vec3(1.0f, 0.95f, 0.7f) * (0.35f + 0.65f * lampIntensity);

        for (const auto& lamp : lamps) {
            glm::vec3 bulbPos = lamp.position + glm::vec3(
                1.2f * cosf(lamp.rotationY),
                4.88f, // lowered so the bulb hangs visibly underneath the shade
                -1.2f * sinf(lamp.rotationY)
            );
            sphere.drawEmissive(unlitShader, I,
                bulbPos.x - 0.08f, bulbPos.y - 0.08f, bulbPos.z - 0.08f,
                0.16f, 0.16f, 0.16f,
                bulbColor);
        }
    }

    // Get spotlight data for light system
    struct SpotLightData {
        glm::vec3 position;
        glm::vec3 direction;
    };

    SpotLightData getLampSpotlight(int idx) const {
        const auto& lamp = lamps[idx];
        glm::vec3 bulbPos = lamp.position + glm::vec3(
            1.2f * cosf(lamp.rotationY),
            4.88f,
            -1.2f * sinf(lamp.rotationY)
        );
        return { bulbPos, glm::vec3(0.0f, -1.0f, 0.0f) };
    }

    int getLampCount() const { return (int)lamps.size(); }

    void cleanup() {
        armMesh.cleanup();
    }

private:
    static inline const glm::vec3 COL_METAL = glm::vec3(0.50f, 0.48f, 0.46f);

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

    void renderLamp(Shader& shader, const glm::mat4& I,
                    Cube& cube, Cylinder& cyl, Sphere& sphere,
                    unsigned int texMetalGrey, const LampInstance& lamp) const
    {
        float lx = lamp.position.x, lz = lamp.position.z;

        bindTex(shader, texMetalGrey, 3.0f);
        setMaterial(shader, COL_METAL, 0.10f, 0.50f, 0.80f, 64.0f);

        // 1. Pole: cylinder r=0.06m, h=5.5m
        cyl.draw(shader, I, lx - 0.06f, 0.0f, lz - 0.06f, 0.12f, 5.5f, 0.12f, COL_METAL, 64.0f);

        // 2. Bezier arm (rendered as mesh)
        glm::mat4 armModel = glm::translate(I, glm::vec3(lx, 0.0f, lz));
        armModel = glm::rotate(armModel, lamp.rotationY, glm::vec3(0, 1, 0));
        shader.setMat4("model", armModel);
        armMesh.draw();

        // 3. Vintage lamp shade
        // Represents a historic bent-metal streetlight shade
        float hx = lx + 1.2f * cosf(lamp.rotationY);
        float hz = lz - 1.2f * sinf(lamp.rotationY);
        
        // Base connector stem
        cyl.draw(shader, I, hx - 0.04f, 4.95f, hz - 0.04f, 0.08f, 0.2f, 0.08f, COL_METAL, 64.0f);
        
        // Metal shade bowl (squashed sphere)
        sphere.draw(shader, I, hx - 0.22f, 4.92f, hz - 0.22f, 0.44f, 0.08f, 0.44f, COL_METAL * 0.8f, 64.0f);

        unbind(shader);
    }

    void buildArmMesh() {
        // Lamp arm Bezier: curves from pole top downward
        // Control points in local space (origin at pole top)
        glm::vec3 P0(0.0f, 5.5f, 0.0f);
        glm::vec3 P1(0.5f, 5.5f, 0.0f);
        glm::vec3 P2(1.0f, 5.3f, 0.0f);
        glm::vec3 P3(1.2f, 5.1f, 0.0f);

        std::vector<float> verts;
        std::vector<unsigned int> indices;
        BezierTube::generate(P0, P1, P2, P3, 0.035f, 12, 6, verts, indices);
        armMesh.initFromData(verts, indices);
    }

    void placeLamps() {
        lamps.clear();
        float pi = glm::pi<float>();

        // UPDATED: Lagerstrasse (Z = -75.5) - every 20m, alternating sides
        // Extended to cover new camp dimensions
        for (float x = -125.0f; x <= 140.0f; x += 20.0f) {
            int idx = (int)((x + 125.0f) / 20.0f);
            float z = -75.5f + (idx % 2 == 0 ? 4.5f : -4.5f);
            float rot = (idx % 2 == 0) ? pi * 0.5f : -pi * 0.5f; // face road
            lamps.push_back({ glm::vec3(x, 0.0f, z), rot });
        }

        // UPDATED: Inter-row roads - synchronized with new row positions
        // Row Z-centres: -54, -18, +18, +54
        // Road positions: -36, 0, +36, +70
        float rowRoadZ[] = { -36.0f, 0.0f, +36.0f, +70.0f };
        for (int r = 0; r < 4; r++) {
            for (float x = -120.0f; x <= 130.0f; x += 24.0f) {
                lamps.push_back({ glm::vec3(x, 0.0f, rowRoadZ[r] + 3.0f), -pi * 0.5f });
            }
        }

        // UPDATED: Appellplatz perimeter - centered in camp core at X=+8, Z=0
        float appCX = 8.0f, appCZ = 0.0f;
        float appW = 48.0f * 0.5f, appH = 82.0f * 0.5f;
        lamps.push_back({ glm::vec3(appCX - appW + 3.0f, 0, appCZ - appH + 3.0f), pi * 0.25f });
        lamps.push_back({ glm::vec3(appCX + appW - 3.0f, 0, appCZ - appH + 3.0f), pi * 0.75f });
        lamps.push_back({ glm::vec3(appCX - appW + 3.0f, 0, appCZ + appH - 3.0f), -pi * 0.25f });
        lamps.push_back({ glm::vec3(appCX + appW - 3.0f, 0, appCZ + appH - 3.0f), -pi * 0.75f });
        lamps.push_back({ glm::vec3(appCX - appW + 3.0f, 0, appCZ), 0.0f });
        lamps.push_back({ glm::vec3(appCX + appW - 3.0f, 0, appCZ), pi });
        lamps.push_back({ glm::vec3(appCX, 0, appCZ - appH + 3.0f), pi * 0.5f });
        lamps.push_back({ glm::vec3(appCX, 0, appCZ + appH - 3.0f), -pi * 0.5f });

        // UPDATED: Near gate - positioned for gate at X=+155
        // Gate entrance at X=+155, Z=0
        lamps.push_back({ glm::vec3(150.0f, 0, -6.0f), pi * 0.75f });
        lamps.push_back({ glm::vec3(150.0f, 0, +6.0f), -pi * 0.75f });
        lamps.push_back({ glm::vec3(145.0f, 0, -4.0f), pi });
        lamps.push_back({ glm::vec3(145.0f, 0, +4.0f), pi });
    }
};

#endif
