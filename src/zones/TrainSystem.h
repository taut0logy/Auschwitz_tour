#ifndef TRAINSYSTEM_H
#define TRAINSYSTEM_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <algorithm>
#include "Shader.h"
#include "Flyweight.h"
#include "primitives/BezierTube.h"
#include "primitives/Cube.h"
#include "primitives/Cylinder.h"
#include "primitives/Sphere.h"

// ================================================================
// TrainSystem:
// - Straight rail line outside the eastern entrance fence
// - One locomotive + five bogeys
// - Scripted motion: appear from horizon -> stop at gate -> depart
// - Keyboard-controlled trigger + speed scaling
// ================================================================
class TrainSystem {
public:
    enum class MotionState {
        Hidden,
        Approaching,
        StoppedAtGate,
        Departing
    };

    struct HeadlightData {
        glm::vec3 position;
        glm::vec3 direction;
        bool active;
    };

    // Line runs parallel to the entrance fence (east side, outside perimeter).
    static constexpr float TRACK_CENTER_X = 176.0f;
    static constexpr float RAIL_GAUGE = 2.2f;
    static constexpr float TRACK_START_Z = -380.0f;
    static constexpr float TRACK_END_Z = 380.0f;
    static constexpr float GATE_STOP_Z = 0.0f;

    void init() {
        buildEngineBoilerMesh();
    }

    void update(float dt) {
        if (state == MotionState::Hidden) return;

        if (state == MotionState::Approaching) {
            frontZ += speedMetersPerSecond * dt;
            if (frontZ >= GATE_STOP_Z) {
                frontZ = GATE_STOP_Z;
                state = MotionState::StoppedAtGate;
                stopTimer = 0.0f;
            }
            return;
        }

        if (state == MotionState::StoppedAtGate) {
            stopTimer += dt;
            if (stopTimer >= stopDurationSeconds) {
                state = MotionState::Departing;
            }
            return;
        }

        if (state == MotionState::Departing) {
            frontZ += speedMetersPerSecond * dt;
            if (frontZ > TRACK_END_Z) {
                state = MotionState::Hidden;
            }
        }
    }

    void renderTrack(Shader& shader, const glm::mat4& I,
                     Cube& cube,
                     unsigned int texMetalGrey,
                     unsigned int texWoodPlank) const
    {
        const glm::vec3 colRail(0.50f, 0.50f, 0.52f);
        const glm::vec3 colWood(0.34f, 0.24f, 0.14f);

        // Steel rails
        bindTex(shader, texMetalGrey, 40.0f);
        setMaterial(shader, colRail, 0.10f, 0.56f, 0.95f, 96.0f);
        cube.draw(shader, I,
            TRACK_CENTER_X - RAIL_GAUGE * 0.5f - 0.05f, 0.05f, TRACK_START_Z,
            0.10f, 0.12f, TRACK_END_Z - TRACK_START_Z,
            colRail, 96.0f);
        cube.draw(shader, I,
            TRACK_CENTER_X + RAIL_GAUGE * 0.5f - 0.05f, 0.05f, TRACK_START_Z,
            0.10f, 0.12f, TRACK_END_Z - TRACK_START_Z,
            colRail, 96.0f);

        // Wooden sleepers between rails
        bindTex(shader, texWoodPlank, 1.0f);
        setMaterial(shader, colWood, 0.16f, 0.68f, 0.06f, 8.0f);
        const float sleeperStep = 2.2f;
        for (float z = TRACK_START_Z; z <= TRACK_END_Z; z += sleeperStep) {
            cube.draw(shader, I,
                TRACK_CENTER_X - 1.55f, 0.0f, z,
                3.10f, 0.10f, 0.35f,
                colWood, 8.0f);
        }

        unbind(shader);
    }

    void renderTrain(Shader& shader, const glm::mat4& I,
                     Cube& cube, Cylinder& cyl, Sphere& sphere,
                     unsigned int texMetalGrey,
                     unsigned int texBlackMetal,
                     unsigned int texWoodPlank,
                     unsigned int texGlassAlpha) const
    {
        if (!isVisible()) return;

        const glm::mat4 base = glm::translate(I, glm::vec3(TRACK_CENTER_X, 0.0f, frontZ));

        renderEngine(shader, base, cube, cyl, sphere,
            texMetalGrey, texBlackMetal, texWoodPlank, texGlassAlpha);

        // Five bogeys trailing behind engine.
        for (int i = 0; i < 5; i++) {
            const float bogeyFrontZ = -(engineLength + couplingGap) - i * (bogeyLength + couplingGap);
            renderBogey(shader, base, cube, sphere, bogeyFrontZ,
                texMetalGrey, texBlackMetal, texWoodPlank, texGlassAlpha, i);
        }
    }

    void renderHeadlightBulb(Shader& unlitShader, const glm::mat4& I,
                             Sphere& sphere,
                             bool isDaytime,
                             float lampIntensity) const
    {
        if (!isVisible()) return;

        const glm::vec3 color = isDaytime
            ? glm::vec3(0.25f, 0.25f, 0.25f)
            : glm::vec3(1.0f, 0.96f, 0.78f) * (0.45f + 0.55f * lampIntensity);

        glm::vec3 p = getHeadlightPosition();
        sphere.drawEmissive(unlitShader, I,
            p.x - 0.20f, p.y - 0.20f, p.z - 0.20f,
            0.40f, 0.40f, 0.40f,
            color);
    }

    HeadlightData getHeadlightData() const {
        HeadlightData h{};
        h.position = getHeadlightPosition();
        h.direction = glm::normalize(glm::vec3(0.0f, -0.12f, 1.0f));
        h.active = isVisible();
        return h;
    }

    void triggerRun() {
        // Always start from outside horizon and perform a full pass.
        frontZ = TRACK_START_Z;
        state = MotionState::Approaching;
        stopTimer = 0.0f;
    }

    void setSpeed(float s) {
        speedMetersPerSecond = glm::clamp(s, 3.0f, 45.0f);
    }

    void adjustSpeed(float delta) {
        setSpeed(speedMetersPerSecond + delta);
    }

    float getSpeed() const {
        return speedMetersPerSecond;
    }

    const char* getStateLabel() const {
        switch (state) {
        case MotionState::Hidden: return "hidden";
        case MotionState::Approaching: return "approaching";
        case MotionState::StoppedAtGate: return "stopped";
        case MotionState::Departing: return "departing";
        default: return "unknown";
        }
    }

    bool isVisible() const {
        return state != MotionState::Hidden;
    }

    void cleanup() {
        engineBoilerMesh.cleanup();
    }

private:
    MeshFlyweight engineBoilerMesh;

    MotionState state = MotionState::Hidden;
    float frontZ = TRACK_START_Z;
    float speedMetersPerSecond = 14.0f;
    float stopTimer = 0.0f;
    float stopDurationSeconds = 5.0f;

    float engineLength = 12.0f;
    float bogeyLength = 7.0f;
    float couplingGap = 1.2f;

    static inline const glm::vec3 COL_ENGINE_DARK = glm::vec3(0.13f, 0.13f, 0.14f);
    static inline const glm::vec3 COL_METAL = glm::vec3(0.50f, 0.48f, 0.46f);
    static inline const glm::vec3 COL_BLACK = glm::vec3(0.12f, 0.12f, 0.12f);
    static inline const glm::vec3 COL_WINDOW = glm::vec3(0.35f, 0.42f, 0.50f);

    glm::vec3 getHeadlightPosition() const {
        return glm::vec3(TRACK_CENTER_X, 2.65f, frontZ + 1.10f);
    }

    void bindTex(Shader& s, unsigned int tex, float rep) const {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex);
        s.setInt("texture1", 0);
        s.setBool("useTexture", tex != 0);
        s.setFloat("texRepeat", rep);
    }

    void unbind(Shader& s) const {
        s.setBool("useTexture", false);
    }

    void setMaterial(Shader& s, glm::vec3 col, float a, float d, float sp, float sh) const {
        s.setVec3("material.ambient", col * a);
        s.setVec3("material.diffuse", col * d);
        s.setVec3("material.specular", glm::vec3(sp));
        s.setFloat("material.shininess", sh);
        s.setVec3("material.emissive", glm::vec3(0.0f));
    }

    void buildEngineBoilerMesh() {
        // Curved boiler spine shaped with cubic Bezier and swept tube.
        glm::vec3 P0(0.0f, 2.05f, -8.6f);
        glm::vec3 P1(0.0f, 2.55f, -6.4f);
        glm::vec3 P2(0.0f, 2.60f, -2.2f);
        glm::vec3 P3(0.0f, 2.25f, 0.6f);

        std::vector<float> verts;
        std::vector<unsigned int> indices;
        BezierTube::generate(P0, P1, P2, P3, 0.74f, 20, 10, verts, indices);
        engineBoilerMesh.initFromData(verts, indices);
    }

    void renderEngine(Shader& shader, const glm::mat4& base,
                      Cube& cube, Cylinder& cyl, Sphere& sphere,
                      unsigned int texMetalGrey,
                      unsigned int texBlackMetal,
                      unsigned int texWoodPlank,
                      unsigned int texGlassAlpha) const
    {
        // Chassis and lower frame
        bindTex(shader, texBlackMetal, 6.0f);
        setMaterial(shader, COL_BLACK, 0.14f, 0.55f, 0.65f, 28.0f);
        cube.draw(shader, base, -1.2f, 0.45f, -11.2f, 2.4f, 0.8f, 11.8f, COL_BLACK, 28.0f);

        // Boiler shell via Bezier tube mesh
        bindTex(shader, texBlackMetal, 3.0f);
        setMaterial(shader, COL_ENGINE_DARK, 0.16f, 0.68f, 0.40f, 20.0f);
        shader.setMat4("model", base);
        engineBoilerMesh.draw();

        // Cab body
        bindTex(shader, texBlackMetal, 3.0f);
        setMaterial(shader, COL_ENGINE_DARK, 0.16f, 0.68f, 0.45f, 24.0f);
        cube.draw(shader, base, -1.45f, 1.25f, -11.9f, 2.9f, 2.2f, 2.9f, COL_ENGINE_DARK, 24.0f);

        // Cab roof and trim
        bindTex(shader, texMetalGrey, 3.0f);
        setMaterial(shader, COL_METAL, 0.12f, 0.58f, 0.75f, 48.0f);
        cube.draw(shader, base, -1.65f, 3.35f, -12.1f, 3.3f, 0.24f, 3.3f, COL_METAL, 48.0f);

        // Front bumper and coupler
        bindTex(shader, texBlackMetal, 3.0f);
        setMaterial(shader, COL_BLACK, 0.12f, 0.56f, 0.55f, 18.0f);
        cube.draw(shader, base, -1.35f, 0.3f, 0.7f, 2.7f, 0.35f, 0.6f, COL_BLACK, 18.0f);
        cube.draw(shader, base, -0.16f, 0.6f, 1.2f, 0.32f, 0.24f, 0.45f, COL_BLACK, 18.0f);

        // Smokestack
        bindTex(shader, texBlackMetal, 3.0f);
        setMaterial(shader, COL_BLACK, 0.16f, 0.56f, 0.36f, 14.0f);
        cyl.draw(shader, base, -0.28f, 2.65f, -2.8f, 0.56f, 1.55f, 0.56f, COL_BLACK, 14.0f);
        bindTex(shader, texMetalGrey, 2.0f);
        setMaterial(shader, COL_METAL, 0.13f, 0.60f, 0.70f, 34.0f);
        cyl.draw(shader, base, -0.42f, 4.10f, -2.95f, 0.84f, 0.24f, 0.84f, COL_METAL, 34.0f);

        // Wheels represented with compact metallic spheres
        bindTex(shader, texBlackMetal, 4.0f);
        setMaterial(shader, COL_BLACK, 0.13f, 0.58f, 0.55f, 18.0f);
        for (float z : { -10.5f, -8.1f, -5.7f, -3.3f, -0.9f }) {
            sphere.draw(shader, base, -1.25f, 0.05f, z, 0.65f, 0.65f, 0.65f, COL_BLACK, 18.0f);
            sphere.draw(shader, base,  0.60f, 0.05f, z, 0.65f, 0.65f, 0.65f, COL_BLACK, 18.0f);
        }

        // Cab windows
        bindTex(shader, texGlassAlpha, 1.0f);
        setMaterial(shader, COL_WINDOW, 0.16f, 0.56f, 0.85f, 64.0f);
        cube.draw(shader, base, -1.38f, 2.08f, -10.3f, 0.10f, 0.9f, 0.9f, COL_WINDOW, 64.0f);
        cube.draw(shader, base,  1.28f, 2.08f, -10.3f, 0.10f, 0.9f, 0.9f, COL_WINDOW, 64.0f);
        cube.draw(shader, base, -0.55f, 2.08f, -9.1f, 1.1f, 0.9f, 0.1f, COL_WINDOW, 64.0f);

        // Headlamp housing
        bindTex(shader, texMetalGrey, 2.0f);
        setMaterial(shader, COL_METAL, 0.15f, 0.62f, 0.80f, 64.0f);
        sphere.draw(shader, base, -0.35f, 2.48f, 0.88f, 0.7f, 0.7f, 0.7f, COL_METAL, 64.0f);

        unbind(shader);
    }

    void renderBogey(Shader& shader, const glm::mat4& base,
                     Cube& cube, Sphere& sphere,
                     float frontZ,
                     unsigned int texMetalGrey,
                     unsigned int texBlackMetal,
                     unsigned int texWoodPlank,
                     unsigned int texGlassAlpha,
                     int index) const
    {
        const glm::vec3 bodyCol = (index % 2 == 0)
            ? glm::vec3(0.15f, 0.15f, 0.16f)
            : glm::vec3(0.12f, 0.12f, 0.13f);

        // Bogey base frame
        bindTex(shader, texBlackMetal, 4.0f);
        setMaterial(shader, COL_BLACK, 0.14f, 0.55f, 0.50f, 18.0f);
        cube.draw(shader, base, -1.25f, 0.45f, frontZ - bogeyLength, 2.5f, 0.75f, bogeyLength, COL_BLACK, 18.0f);

        // Body shell
        bindTex(shader, texBlackMetal, 3.0f);
        setMaterial(shader, bodyCol, 0.14f, 0.70f, 0.28f, 14.0f);
        cube.draw(shader, base, -1.50f, 1.18f, frontZ - bogeyLength + 0.2f, 3.0f, 2.0f, bogeyLength - 0.4f, bodyCol, 10.0f);

        // Roof
        bindTex(shader, texMetalGrey, 2.0f);
        setMaterial(shader, COL_METAL, 0.11f, 0.58f, 0.65f, 20.0f);
        cube.draw(shader, base, -1.65f, 3.02f, frontZ - bogeyLength + 0.05f, 3.3f, 0.18f, bogeyLength - 0.1f, COL_METAL, 20.0f);

        // Side windows
        bindTex(shader, texGlassAlpha, 1.0f);
        setMaterial(shader, COL_WINDOW, 0.15f, 0.58f, 0.80f, 48.0f);
        for (int w = 0; w < 3; w++) {
            float wz = frontZ - bogeyLength + 1.0f + w * 1.8f;
            cube.draw(shader, base, -1.45f, 1.9f, wz, 0.1f, 0.85f, 1.1f, COL_WINDOW, 48.0f);
            cube.draw(shader, base,  1.35f, 1.9f, wz, 0.1f, 0.85f, 1.1f, COL_WINDOW, 48.0f);
        }

        // Wheel sets
        bindTex(shader, texBlackMetal, 4.0f);
        setMaterial(shader, COL_BLACK, 0.13f, 0.56f, 0.55f, 18.0f);
        for (float z : { frontZ - bogeyLength + 1.0f, frontZ - 1.0f }) {
            sphere.draw(shader, base, -1.30f, 0.05f, z, 0.62f, 0.62f, 0.62f, COL_BLACK, 18.0f);
            sphere.draw(shader, base,  0.68f, 0.05f, z, 0.62f, 0.62f, 0.62f, COL_BLACK, 18.0f);
        }

        // Coupler beam
        bindTex(shader, texBlackMetal, 1.0f);
        setMaterial(shader, COL_BLACK, 0.12f, 0.52f, 0.40f, 12.0f);
        cube.draw(shader, base, -0.12f, 0.62f, frontZ + 0.02f, 0.24f, 0.16f, couplingGap - 0.1f, COL_BLACK, 12.0f);

        unbind(shader);
    }
};

#endif
