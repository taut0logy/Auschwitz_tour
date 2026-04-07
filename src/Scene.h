#ifndef SCENE_H
#define SCENE_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>

// Core
#include "Shader.h"
#include "Texture.h"
#include "Camera.h"
#include "Flyweight.h"
#include "DayNightCycle.h"
#include "LSystemTree.h"
#include "ParticleSystem.h"
#include "BezierCurve.h"
#include "LightCuller.h"
#include "HorizonSystem.h"
#include "HUD.h"

// Primitives
#include "primitives/Cube.h"
#include "primitives/Cylinder.h"
#include "primitives/Sphere.h"
#include "primitives/Plane.h"
#include "primitives/Pyramid.h"
#include "primitives/BezierTube.h"
#include "primitives/RuledSurface.h"

// Light classes
#include "lights/DirectionalLight.h"
#include "lights/PointLight.h"
#include "lights/SpotLight.h"

// Zone modules
#include "zones/GroundZone.h"
#include "zones/BarrackGrid.h"
#include "zones/BarrackInteriors.h"
#include "zones/EntranceGate.h"
#include "zones/FenceSystem.h"
#include "zones/GuardTowers.h"
#include "zones/StreetLamps.h"
#include "zones/Block11Zone.h"
#include "zones/CrematoryZone.h"
#include "zones/AdminZone.h"
#include "zones/TrainSystem.h"
#include "zones/SoldiersCourtyard.h"

// ================================================================
// Scene: Central orchestrator for the Auschwitz I reconstruction
// Manages initialization, shaders, textures, lights, zones, and
// the multi-pass render pipeline
// ================================================================
class Scene {
public:
    // ---- Primitives ----
    Cube cube;
    Cylinder cylinder;
    Sphere sphere;
    Plane plane;
    Pyramid pyramid;

    // ---- Subsystems ----
    DayNightCycle dayNight;
    LSystemTree lsystem;
    ParticleSystem particles;
    LightCuller lightCuller;
    HorizonSystem horizon;
    HUD hud;

    // ---- Zones ----
    GroundZone groundZone;
    BarrackGrid barrackGrid;
    BarrackInteriors interiors;
    EntranceGate entranceGate;
    FenceSystem fenceSystem;
    GuardTowers guardTowers;
    StreetLamps streetLamps;
    Block11Zone block11Zone;
    CrematoryZone crematoryZone;
    AdminZone adminZone;
    TrainSystem trainSystem;
    SoldiersCourtyard soldiersCourtyard;

    // ---- Lights ----
    DirectionalLight sunLight;
    DirectionalLight moonLight;
    std::vector<PointLight> interiorLights;
    std::vector<SpotLight> towerSpots;
    std::vector<SpotLight> lampSpots;

    // ---- Skybox ----
    GLuint skyboxVAO = 0, skyboxVBO = 0;

    // ---- Stars ----
    GLuint starVAO = 0, starVBO = 0;
    int starCount = 800;

    // ---- Textures ----
    unsigned int texBrickRed = 0, texBrickDark = 0;
    unsigned int texRoofTile = 0, texConcrete = 0;
    unsigned int texWoodPlank = 0, texWoodDark = 0;
    unsigned int texMetalIron = 0, texMetalGrey = 0;
    unsigned int texDirtRoad = 0, texGravel = 0;
    unsigned int texBark = 0, texLeafAlpha = 0;
    unsigned int texWireAlpha = 0;
    unsigned int texHorizonTrees = 0, texMoon = 0;
    unsigned int texCobblestone = 0, texTileGrey = 0;
    unsigned int texPlasterWhite = 0, texStrawBedding = 0;
    unsigned int texBlackMetal = 0, texStoneFloor = 0;
    unsigned int texTextArbeit = 0, texGlassAlpha = 0;

    void init() {
        // ---- Init primitives ----
        cube.init();
        cylinder.init(36);
        sphere.init(36, 18);
        plane.init();
        pyramid.init();

        // ---- Load textures ----
        loadTextures();

        // ---- Init subsystems ----
        entranceGate.init();
        fenceSystem.init();
        crematoryZone.init();
        streetLamps.init();
        trainSystem.init();
        soldiersCourtyard.init();
        lsystem.init();
        particles.init();
        horizon.init(texHorizonTrees);
        hud.init();

        // ---- Build skybox ----
        buildSkybox();
        buildStars();

        // ---- Setup lights ----
        setupLights();
    }

    // =========================================================
    // RENDER PASS 1: Skybox
    // =========================================================
    void renderSkybox(Shader& skyboxShader, const glm::mat4& view, const glm::mat4& proj) {
        glDepthFunc(GL_LEQUAL);
        skyboxShader.use();
        skyboxShader.setMat4("view", view);
        skyboxShader.setMat4("projection", proj);
        skyboxShader.setVec3("skyTopColor", dayNight.getTopColor());
        skyboxShader.setVec3("skyHorizonColor", dayNight.getHorizonColor());
        skyboxShader.setVec3("sunGlowDir", glm::normalize(-dayNight.getSunDirection()));
        skyboxShader.setFloat("sunGlowStrength", dayNight.getSunGlowStrength());

        glBindVertexArray(skyboxVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS);
    }

    // =========================================================
    // RENDER PASS 2: Stars
    // =========================================================
    void renderStars(Shader& unlitShader, const glm::mat4& view, const glm::mat4& proj) {
        if (dayNight.starAlpha < 0.01f) return;

        unlitShader.use();
        unlitShader.setMat4("view", view);
        unlitShader.setMat4("projection", proj);
        unlitShader.setMat4("model", glm::mat4(1.0f));
        unlitShader.setVec3("objectColor", glm::vec3(1.0f, 1.0f, 0.95f) * dayNight.starAlpha);

        glEnable(GL_PROGRAM_POINT_SIZE);
        glBindVertexArray(starVAO);
        glDrawArrays(GL_POINTS, 0, starCount);
        glBindVertexArray(0);
    }

    // =========================================================
    // RENDER PASS 3: Opaque geometry
    // =========================================================
    void renderOpaque(Shader& phongShader, const Camera& camera,
                      const glm::mat4& view, const glm::mat4& proj) {
        glm::mat4 I(1.0f);

        phongShader.use();
        phongShader.setMat4("view", view);
        phongShader.setMat4("projection", proj);
        phongShader.setVec3("viewPos", camera.position);
        phongShader.setFloat("textureBlend", 0.85f);

        // Upload lights
        uploadLights(phongShader, camera, view, proj);

        // ---- Draw all opaque zones ----
        groundZone.render(phongShader, I, cube, plane, texGravel, texDirtRoad, texCobblestone, texConcrete);
        barrackGrid.render(phongShader, I, cube, cylinder, plane,
            texBrickRed, texBrickDark, texRoofTile, texConcrete,
            texWoodPlank, texWoodDark, texGlassAlpha,
            doorOpenAmount, windowOpenAmount);
        entranceGate.render(phongShader, I, cube, cylinder, plane, pyramid,
            texBrickDark, texBrickRed, texRoofTile, texMetalIron,
            texCobblestone, texWoodDark, texTextArbeit);
        trainSystem.renderTrack(phongShader, I, cube, texMetalGrey, texWoodPlank);
        fenceSystem.renderPosts(phongShader, I, cylinder, texConcrete);
        guardTowers.render(phongShader, I, cube, cylinder,
            texWoodPlank, texRoofTile, texMetalGrey, texConcrete);
        streetLamps.render(phongShader, I, cube, cylinder, sphere, texMetalGrey);
        block11Zone.render(phongShader, I, cube, plane, texBrickDark, texWoodDark, texGravel);
        crematoryZone.render(phongShader, I, cube, cylinder, plane,
            texBrickDark, texRoofTile, texConcrete, texGravel, texWoodDark);
        adminZone.render(phongShader, I, cube, texBrickRed, texRoofTile);
        soldiersCourtyard.render(phongShader, I, cube, sphere, plane);
        trainSystem.renderTrain(phongShader, I, cube, cylinder, sphere,
            texMetalGrey, texBlackMetal, texWoodPlank, texGlassAlpha);

        // Horizon ground extension
        horizon.renderGroundExtension(phongShader, I, plane, texGravel);

        // Tree branches (opaque)
        lsystem.renderBranches(phongShader, I, texBark);

        // Fence wire (thin lines, considered opaque)
        phongShader.setBool("useTexture", false);
        phongShader.setVec3("material.ambient", glm::vec3(0.05f));
        phongShader.setVec3("material.diffuse", glm::vec3(0.45f, 0.42f, 0.40f));
        phongShader.setVec3("material.specular", glm::vec3(0.8f));
        phongShader.setFloat("material.shininess", 64.0f);
        phongShader.setVec3("material.emissive", glm::vec3(0.0f));
        fenceSystem.renderWire(phongShader, I);

        // Barrack interiors (proximity-gated)
        interiors.render(phongShader, I, cube, cylinder, sphere, plane, camera,
            texWoodPlank, texBrickDark, texStoneFloor, texPlasterWhite,
            texStrawBedding, texBlackMetal, texTileGrey, texRoofTile);
    }

    // =========================================================
    // RENDER PASS 4: Celestial bodies (sun/moon)
    // =========================================================
    void renderCelestial(Shader& unlitShader, const glm::mat4& view, const glm::mat4& proj) {
        unlitShader.use();
        unlitShader.setMat4("view", view);
        unlitShader.setMat4("projection", proj);

        // Sun
        if (dayNight.getSunIntensity() > 0.01f) {
            glm::vec3 sunPos = dayNight.getSunPosition();
            sphere.drawEmissive(unlitShader, glm::mat4(1.0f),
                sunPos.x - 6, sunPos.y - 6, sunPos.z - 6,
                12, 12, 12, glm::vec3(1.0f, 0.95f, 0.7f));
        }

        // Moon
        if (dayNight.getMoonIntensity() > 0.01f) {
            glm::vec3 moonPos = dayNight.getMoonPosition();
            unlitShader.setBool("useTexture", true);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texMoon);
            unlitShader.setInt("texture1", 0);
            
            // Draw a much larger moon (e.g. radius 20m means scale 40, offset 20)
            sphere.drawEmissive(unlitShader, glm::mat4(1.0f),
                moonPos.x - 20.0f, moonPos.y - 20.0f, moonPos.z - 20.0f,
                40.0f, 40.0f, 40.0f, glm::vec3(0.9f, 0.95f, 1.0f) * 1.5f);
                
            unlitShader.setBool("useTexture", false);
        }

        // Street lamp bulbs (gray in daytime, warm at night)
        streetLamps.renderBulbs(unlitShader, glm::mat4(1.0f), sphere,
            dayNight.isDaytime(), dayNight.lampIntensity);
        trainSystem.renderHeadlightBulb(unlitShader, glm::mat4(1.0f), sphere,
            dayNight.isDaytime(), dayNight.lampIntensity);

        // Interior bulbs
        interiors.renderBulbs(unlitShader, glm::mat4(1.0f), sphere, *currentCamera);
    }

    // =========================================================
    // RENDER PASS 5: Alpha / transparent objects
    // =========================================================
    void renderAlpha(Shader& alphaShader, const Camera& camera,
                     const glm::mat4& view, const glm::mat4& proj) {
        glm::mat4 I(1.0f);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        alphaShader.use();
        alphaShader.setMat4("view", view);
        alphaShader.setMat4("projection", proj);
        alphaShader.setVec3("viewPos", camera.position);
        alphaShader.setFloat("objectAlpha", 1.0f);
        glm::vec3 nightAmbient = glm::vec3(0.04f, 0.04f, 0.06f) * dayNight.getNightFactor();
        alphaShader.setVec3("globalAmbient", dayNight.getTopColor() * 0.1f + nightAmbient);
        
        float si = dayNight.getSunIntensity();
        float mi = dayNight.getMoonIntensity();
        if (si > mi) {
            alphaShader.setVec3("dirLight.direction", dayNight.getSunDirection());
            alphaShader.setVec3("dirLight.ambient", glm::vec3(0.2f * si));
            alphaShader.setVec3("dirLight.diffuse", glm::vec3(0.8f * si));
            alphaShader.setVec3("dirLight.specular", glm::vec3(0.3f * si));
        } else {
            alphaShader.setVec3("dirLight.direction", dayNight.getMoonDirection());
            alphaShader.setVec3("dirLight.ambient", glm::vec3(0.05f * mi));
            alphaShader.setVec3("dirLight.diffuse", glm::vec3(0.2f * mi));
            alphaShader.setVec3("dirLight.specular", glm::vec3(0.05f * mi));
        }
        
        alphaShader.setFloat("texRepeat", 1.0f);

        // Tree leaves
        lsystem.renderLeaves(alphaShader, I, texLeafAlpha);

        // Horizon alpha layers
        horizon.renderAlphaLayers(alphaShader, I, texHorizonTrees, dayNight.fogColor);

        // Ground fog
        horizon.renderGroundFog(alphaShader, I, cube, dayNight.getNightFactor());

        glDisable(GL_BLEND);
    }

    // =========================================================
    // RENDER PASS 6: Particles
    // =========================================================
    void renderParticles(Shader& particleShader, const Camera& camera,
                         const glm::mat4& view, const glm::mat4& proj) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
        particles.renderSnowflakes(particleShader, view, proj);
        particles.renderSmoke(particleShader, view, proj);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    }

    // =========================================================
    // RENDER PASS 7: HUD
    // =========================================================
    void renderHUD(int winW, int winH) {
        hud.render(winW, winH, dayNight.timeOfDay, *currentCamera);
    }

    // =========================================================
    // Update
    // =========================================================
    void update(float deltaTime, const Camera& camera) {
        currentCamera = &camera;
        dayNight.update(deltaTime);
        soldiersCourtyard.update(deltaTime);
        trainSystem.update(deltaTime);
        particles.update(deltaTime, dayNight.timeOfDay);
        updateLights();
        updateAnimation(deltaTime);
    }

    void toggleSoldierParade() {
        soldiersCourtyard.toggleParade();
    }

    bool isSoldierParadeEnabled() const {
        return soldiersCourtyard.isParadeEnabled();
    }

    void triggerTrainRun() {
        trainSystem.triggerRun();
    }

    void adjustTrainSpeed(float delta) {
        trainSystem.adjustSpeed(delta);
    }

    float getTrainSpeed() const {
        return trainSystem.getSpeed();
    }

    const char* getTrainStateLabel() const {
        return trainSystem.getStateLabel();
    }

    // ---- Animation state for interactive elements ----
    bool doorOpening = false;
    bool doorClosing = false;
    float doorOpenAmount = 0.0f; // 0.0 = closed, 1.0 = fully open (90 degrees)
    
    bool windowOpening = false;
    bool windowClosing = false;
    float windowOpenAmount = 0.0f; // 0.0 = closed, 1.0 = fully open (0.5m slide)

    void updateAnimation(float deltaTime) {
        auto step = [&](bool opening, bool closing, float& amt) {
            if (opening && amt < 1.0f) amt += 1.4f * deltaTime;
            if (closing && amt > 0.0f) amt -= 1.4f * deltaTime;
            amt = glm::clamp(amt, 0.0f, 1.0f);
        };
        step(doorOpening, doorClosing, doorOpenAmount);
        step(windowOpening, windowClosing, windowOpenAmount);
    }

    // Store window dimensions for shadow pass viewport restore
    int windowWidth = 1920, windowHeight = 1080;
    const Camera* currentCamera = nullptr;

    void cleanup() {
        cube.cleanup(); cylinder.cleanup(); sphere.cleanup(); plane.cleanup();
        pyramid.cleanup();
        entranceGate.cleanup(); fenceSystem.cleanup();
        crematoryZone.cleanup(); streetLamps.cleanup();
        trainSystem.cleanup();
        lsystem.cleanup(); particles.cleanup();
        horizon.cleanup(); hud.cleanup();
        if (skyboxVAO) { glDeleteVertexArrays(1, &skyboxVAO); glDeleteBuffers(1, &skyboxVBO); }
        if (starVAO) { glDeleteVertexArrays(1, &starVAO); glDeleteBuffers(1, &starVBO); }
    }

private:
    // ---- Texture loading ----
    void loadTextures() {
        auto load = [&](const char* path, float r, float g, float b) -> unsigned int {
            return loadOrGenerate(path, r, g, b);
        };
        auto loadAlpha = [&](const char* path, float r, float g, float b) -> unsigned int {
            unsigned int t = loadTexture(path, GL_CLAMP_TO_EDGE);
            if (t) return t;
            return generateSolidTexture(r, g, b);
        };

        texBrickRed     = load("textures/brick_red.png", 0.6f, 0.2f, 0.1f);
        texBrickDark    = load("textures/brick_dark.png", 0.35f, 0.17f, 0.05f);
        texRoofTile     = load("textures/roof_tile.png", 0.35f, 0.3f, 0.25f);
        texConcrete     = load("textures/concrete.png", 0.6f, 0.58f, 0.55f);
        texWoodPlank    = load("textures/wood_plank.png", 0.45f, 0.3f, 0.15f);
        texDirtRoad     = load("textures/dirt_road.png", 0.45f, 0.38f, 0.28f);
        texGravel       = load("textures/gravel.png", 0.4f, 0.38f, 0.35f);
        texMetalIron    = load("textures/metal_iron.png", 0.3f, 0.3f, 0.3f);
        texBark         = load("textures/bark.png", 0.25f, 0.2f, 0.15f);
        texHorizonTrees = loadAlpha("textures/horizon_trees.png", 0.2f, 0.25f, 0.15f);
        texMoon         = loadAlpha("textures/moon.png", 0.8f, 0.85f, 0.9f);
        texLeafAlpha    = loadAlpha("textures/leaf_alpha.png", 0.2f, 0.4f, 0.1f);
        texWireAlpha    = loadAlpha("textures/wire_alpha.png", 0.5f, 0.5f, 0.5f);

        // Missing textures: generate solid-colour procedural fallbacks
        texWoodDark     = loadOrGenerate("textures/wood_dark.png",     0.30f, 0.18f, 0.08f);
        texMetalGrey    = loadOrGenerate("textures/metal_grey.png",    0.50f, 0.48f, 0.46f);
        texCobblestone  = loadOrGenerate("textures/cobblestone.png",   0.45f, 0.42f, 0.40f);
        texTileGrey     = loadOrGenerate("textures/tile_grey.png",     0.55f, 0.55f, 0.53f);
        texPlasterWhite = loadOrGenerate("textures/plaster_white.png", 0.85f, 0.82f, 0.80f);
        texStrawBedding = loadOrGenerate("textures/straw_bedding.png", 0.70f, 0.60f, 0.35f);
        texBlackMetal   = loadOrGenerate("textures/black_metal.png",   0.15f, 0.15f, 0.15f);
        texStoneFloor   = loadOrGenerate("textures/stone_floor.png",   0.35f, 0.33f, 0.30f);
        texTextArbeit   = loadOrGenerate("textures/text_arbeit.png",   0.10f, 0.10f, 0.10f);
        texGlassAlpha   = loadOrGenerate("textures/glass_alpha.png",   0.30f, 0.35f, 0.40f);
    }

    unsigned int loadOrGenerate(const char* path, float r, float g, float b) {
        unsigned int tex = loadTexture(path, GL_REPEAT);
        if (tex) return tex;
        // Generate solid-colour 4x4 texture
        return generateSolidTexture(r, g, b);
    }

    unsigned int generateSolidTexture(float r, float g, float b) {
        unsigned char pixels[4 * 4 * 4]; // 4x4 RGBA
        for (int i = 0; i < 16; i++) {
            pixels[i * 4 + 0] = (unsigned char)(r * 255);
            pixels[i * 4 + 1] = (unsigned char)(g * 255);
            pixels[i * 4 + 2] = (unsigned char)(b * 255);
            pixels[i * 4 + 3] = 255;
        }
        unsigned int tex;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        return tex;
    }

    // ---- Light setup ----
    void setupLights() {
        // Sun
        sunLight = DirectionalLight(
            dayNight.getSunDirection(),
            glm::vec3(0.15f), glm::vec3(0.85f), glm::vec3(0.3f));

        // Moon
        moonLight = DirectionalLight(
            dayNight.getMoonDirection(),
            glm::vec3(0.03f), glm::vec3(0.12f), glm::vec3(0.05f));

        // Tower spotlights (12)
        for (int i = 0; i < GuardTowers::NUM_TOWERS; i++) {
            auto t = guardTowers.towers[i];
            towerSpots.push_back(SpotLight(
                i, glm::vec3(t.x, 7.2f, t.z),
                glm::vec3(0.0f, -0.8f, 0.0f),   // aim downward
                glm::vec3(0.035f), glm::vec3(3.8f, 3.6f, 3.0f), glm::vec3(0.9f),
                1.0f, 0.014f, 0.0007f,
                20.0f, 28.0f));
        }

        // Lamp spotlights (dynamically assigned based on culling)
        for (int i = 0; i < streetLamps.getLampCount(); i++) {
            auto data = streetLamps.getLampSpotlight(i);
            SpotLight sl(12 + i, data.position, data.direction,
                glm::vec3(0.015f), glm::vec3(2.0f, 1.85f, 1.5f), glm::vec3(0.45f),
                1.0f, 0.055f, 0.0105f,
                35.0f, 45.0f);
            lampSpots.push_back(sl);
        }

        // Interior point lights
        for (int blockNum = 1; blockNum <= 24; blockNum++) {
            glm::vec3 sw = BarrackGrid::getBlockSW(blockNum);
            for (float lx : {8.0f, 16.0f, 24.0f, 32.0f}) {
                glm::vec3 pos = glm::vec3(sw.x + lx, 6.8f, sw.z + 5.5f);
                interiorLights.push_back(PointLight(
                    0, pos,
                    glm::vec3(0.1f), // ambient
                    glm::vec3(2.5f, 2.2f, 1.5f), // diffuse (warm, bright)
                    glm::vec3(0.5f), // specular
                    1.0f, 0.09f, 0.032f // attenuation (~50m range)
                ));
            }
        }
    }

    void updateLights() {
        // Update sun/moon from day/night cycle
        float si = dayNight.getSunIntensity();
        sunLight.direction = dayNight.getSunDirection();
        sunLight.ambient = glm::vec3(0.15f * si);
        sunLight.diffuse = glm::vec3(0.85f * si);
        sunLight.specular = glm::vec3(0.3f * si);

        float mi = dayNight.getMoonIntensity();
        moonLight.direction = dayNight.getMoonDirection();
        moonLight.ambient = glm::vec3(0.03f * mi);
        moonLight.diffuse = glm::vec3(0.12f * mi);
        moonLight.specular = glm::vec3(0.05f * mi);

        // Guard tower searchlights sweep at night like real search beams.
        // During daytime they park in a downward pose.
        const bool isNight = !dayNight.isDaytime();
        const float t = dayNight.timeOfDay;
        for (int i = 0; i < (int)towerSpots.size(); i++) {
            if (!isNight) {
                towerSpots[i].direction = glm::normalize(glm::vec3(0.0f, -0.9f, 0.0f));
                continue;
            }

            const float base = (t / 24.0f) * glm::two_pi<float>() * 2.0f;
            const float phase = i * (glm::two_pi<float>() / 12.0f);
            const float a = base + phase;
            const float pitch = -0.62f + 0.08f * sinf(base * 0.65f + phase);
            towerSpots[i].direction = glm::normalize(glm::vec3(
                cosf(a) * 0.82f,
                pitch,
                sinf(a) * 0.82f
            ));
        }
    }

    void uploadLights(Shader& shader, const Camera& camera,
                      const glm::mat4& view, const glm::mat4& proj) {
        // Base ambient to prevent pitch black + dynamic ambient
        glm::vec3 nightAmbient = glm::vec3(0.07f, 0.07f, 0.09f) * dayNight.getNightFactor();
        shader.setVec3("globalAmbient", dayNight.getTopColor() * 0.08f + nightAmbient);

        // Directional lights
        shader.setVec3("dirLights[0].direction", sunLight.direction);
        shader.setVec3("dirLights[0].ambient", sunLight.ambient);
        shader.setVec3("dirLights[0].diffuse", sunLight.diffuse);
        shader.setVec3("dirLights[0].specular", sunLight.specular);

        shader.setVec3("dirLights[1].direction", moonLight.direction);
        shader.setVec3("dirLights[1].ambient", moonLight.ambient);
        shader.setVec3("dirLights[1].diffuse", moonLight.diffuse);
        shader.setVec3("dirLights[1].specular", moonLight.specular);

        // Point lights (interior lighting)
        std::vector<glm::vec3> interiorPos;
        for (const auto& l : interiorLights) interiorPos.push_back(l.position);

        auto visibleInteriors = lightCuller.cullLamps(camera.position,
            interiorPos.data(), (int)interiorPos.size(), 32); // Max 32 point lights

        int pointIdx = 0;
        for (int vi = 0; vi < (int)visibleInteriors.size() && pointIdx < 32; vi++, pointIdx++) {
            int lampIdx = visibleInteriors[vi];
            const auto& pl = interiorLights[lampIdx];
            std::string base = "pointLights[" + std::to_string(pointIdx) + "].";
            shader.setVec3(base + "position",   pl.position);
            shader.setVec3(base + "ambient",    pl.ambient * dayNight.lampIntensity);
            shader.setVec3(base + "diffuse",    pl.diffuse * dayNight.lampIntensity);
            shader.setVec3(base + "specular",   pl.specular * dayNight.lampIntensity);
            shader.setFloat(base + "k_c",       pl.k_c);
            shader.setFloat(base + "k_l",       pl.k_l);
            shader.setFloat(base + "k_q",       pl.k_q);
        }
        for (; pointIdx < 32; pointIdx++) {
            std::string base = "pointLights[" + std::to_string(pointIdx) + "].";
            shader.setVec3(base + "ambient", glm::vec3(0));
            shader.setVec3(base + "diffuse", glm::vec3(0));
            shader.setVec3(base + "specular", glm::vec3(0));
        }
        shader.setInt("activePointLights", pointIdx);

        // Spot lights — distance-only culling for lamps.
        // Keep nearby lamps active regardless of camera frustum so night lighting stays stable.
        std::vector<std::pair<float, int>> lampDistances;
        lampDistances.reserve(lampSpots.size());
        for (int i = 0; i < (int)lampSpots.size(); i++) {
            const glm::vec3 d = lampSpots[i].position - camera.position;
            const float distSq = glm::dot(d, d);
            lampDistances.push_back({ distSq, i });
        }
        std::sort(lampDistances.begin(), lampDistances.end(),
            [](const std::pair<float, int>& a, const std::pair<float, int>& b) {
                return a.first < b.first;
            });

        std::vector<int> visibleLamps;
        int keep = std::min((int)lampDistances.size(), 23);
        visibleLamps.reserve(keep);
        for (int i = 0; i < keep; i++)
            visibleLamps.push_back(lampDistances[i].second);

        int spotIdx = 0;
        const bool isNight = !dayNight.isDaytime();
        float towerIntensity = isNight ? (0.30f + 0.40f * dayNight.lampIntensity) : 0.0f;
        float lampIntensity = isNight ? (0.05f + 0.55f * dayNight.lampIntensity) : 0.0f;

        // Tower spotlights (always active, first 12 slots)
        for (int i = 0; i < (int)towerSpots.size() && spotIdx < 36; i++, spotIdx++) {
            std::string base = "spotLights[" + std::to_string(spotIdx) + "].";
            shader.setVec3(base + "position",   towerSpots[i].position);
            shader.setVec3(base + "direction",  towerSpots[i].direction);
            shader.setVec3(base + "ambient",    towerSpots[i].ambient * towerIntensity);
            shader.setVec3(base + "diffuse",    towerSpots[i].diffuse * towerIntensity);
            shader.setVec3(base + "specular",   towerSpots[i].specular * towerIntensity);
            shader.setFloat(base + "k_c",       towerSpots[i].k_c);
            shader.setFloat(base + "k_l",       towerSpots[i].k_l);
            shader.setFloat(base + "k_q",       towerSpots[i].k_q);
            shader.setFloat(base + "cutOff",    towerSpots[i].cutOff);
            shader.setFloat(base + "outerCutOff", towerSpots[i].outerCutOff);
        }

        // Culled lamp spotlights (up to 23, one spotlight slot reserved for train headlight)
        for (int vi = 0; vi < (int)visibleLamps.size() && spotIdx < 36; vi++, spotIdx++) {
            int lampIdx = visibleLamps[vi];
            const auto& ls = lampSpots[lampIdx];
            std::string base = "spotLights[" + std::to_string(spotIdx) + "].";
            shader.setVec3(base + "position",   ls.position);
            shader.setVec3(base + "direction",  ls.direction);
            shader.setVec3(base + "ambient",    ls.ambient * lampIntensity);
            shader.setVec3(base + "diffuse",    ls.diffuse * lampIntensity);
            shader.setVec3(base + "specular",   ls.specular * lampIntensity);
            shader.setFloat(base + "k_c",       ls.k_c);
            shader.setFloat(base + "k_l",       ls.k_l);
            shader.setFloat(base + "k_q",       ls.k_q);
            shader.setFloat(base + "cutOff",    ls.cutOff);
            shader.setFloat(base + "outerCutOff", ls.outerCutOff);
        }

        // Train headlight spotlight
        if (spotIdx < 36) {
            auto trainHead = trainSystem.getHeadlightData();
            std::string base = "spotLights[" + std::to_string(spotIdx) + "].";
            float trainIntensity = (!dayNight.isDaytime() && trainHead.active)
                ? (0.25f + 0.95f * dayNight.lampIntensity)
                : 0.0f;

            shader.setVec3(base + "position", trainHead.position);
            shader.setVec3(base + "direction", trainHead.direction);
            shader.setVec3(base + "ambient", glm::vec3(0.02f, 0.02f, 0.02f) * trainIntensity);
            shader.setVec3(base + "diffuse", glm::vec3(2.4f, 2.2f, 1.7f) * trainIntensity);
            shader.setVec3(base + "specular", glm::vec3(0.75f) * trainIntensity);
            shader.setFloat(base + "k_c", 1.0f);
            shader.setFloat(base + "k_l", 0.05f);
            shader.setFloat(base + "k_q", 0.009f);
            shader.setFloat(base + "cutOff", glm::cos(glm::radians(20.0f)));
            shader.setFloat(base + "outerCutOff", glm::cos(glm::radians(28.0f)));
            spotIdx++;
        }

        // Zero remaining spots
        for (; spotIdx < 36; spotIdx++) {
            std::string base = "spotLights[" + std::to_string(spotIdx) + "].";
            shader.setVec3(base + "ambient", glm::vec3(0));
            shader.setVec3(base + "diffuse", glm::vec3(0));
            shader.setVec3(base + "specular", glm::vec3(0));
        }

        shader.setInt("activeSpotLights", spotIdx);
    }



    // ---- Skybox cube ----
    void buildSkybox() {
        float skyboxVertices[] = {
            -1, 1,-1,  -1,-1,-1,  1,-1,-1,  1,-1,-1,   1, 1,-1,  -1, 1,-1,
            -1,-1, 1,  -1,-1,-1,  -1, 1,-1,  -1, 1,-1,  -1, 1, 1,  -1,-1, 1,
             1,-1,-1,   1,-1, 1,   1, 1, 1,   1, 1, 1,   1, 1,-1,   1,-1,-1,
            -1,-1, 1,  -1, 1, 1,   1, 1, 1,   1, 1, 1,   1,-1, 1,  -1,-1, 1,
            -1, 1,-1,   1, 1,-1,   1, 1, 1,   1, 1, 1,  -1, 1, 1,  -1, 1,-1,
            -1,-1,-1,  -1,-1, 1,   1,-1, 1,   1,-1, 1,   1,-1,-1,  -1,-1,-1,
        };
        glGenVertexArrays(1, &skyboxVAO);
        glGenBuffers(1, &skyboxVBO);
        glBindVertexArray(skyboxVAO);
        glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
    }

    // ---- Stars ----
    void buildStars() {
        std::vector<float> verts;
        srand(42);
        float R = 600.0f;
        for (int i = 0; i < starCount; i++) {
            // Random point on upper hemisphere
            float theta = ((float)rand() / RAND_MAX) * 2.0f * 3.14159f;
            float phi = ((float)rand() / RAND_MAX) * 0.45f * 3.14159f;  // 0 to 81 degrees
            float x = R * cosf(phi) * cosf(theta);
            float y = R * sinf(phi);
            float z = R * cosf(phi) * sinf(theta);
            verts.insert(verts.end(), {x, y, z});
        }

        glGenVertexArrays(1, &starVAO);
        glGenBuffers(1, &starVBO);
        glBindVertexArray(starVAO);
        glBindBuffer(GL_ARRAY_BUFFER, starVBO);
        glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
    }
};

#endif
