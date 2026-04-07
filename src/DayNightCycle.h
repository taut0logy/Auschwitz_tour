#ifndef DAYNIGHTCYCLE_H
#define DAYNIGHTCYCLE_H

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <cmath>
#include <algorithm>
#include <string>

// ================================================================
// DayNightCycle: Time-of-day, sky interpolation, fog color,
// sun/moon positions, star visibility, lamp activation fade
// Per Sections 4.1, 14
// ================================================================
class DayNightCycle {
public:
    float timeOfDay = 8.0f;  // 0.0–24.0
    float timeSpeed = 1.0f;  // 0, 1, 8, 30
    int speedMode = 1;       // 0=paused, 1=1x, 2=8x, 3=30x

    // Lamp activation (0.0=off, 1.0=full)
    float lampIntensity = 0.0f;

    // Star visibility (0.0=invisible, 1.0=full)
    float starAlpha = 0.0f;

    // Fog parameters
    glm::vec3 fogColor = glm::vec3(0.6f, 0.6f, 0.6f);
    float fogStart = 100.0f;
    float fogEnd = 350.0f;

    struct SkyKeyframe {
        float hour;
        glm::vec3 top;
        glm::vec3 horizon;
    };

    // v2 specification keyframes (hex converted to float)
    static constexpr int NUM_KEYFRAMES = 9;
    SkyKeyframe keyframes[NUM_KEYFRAMES] = {
        {  0.0f, {0.020f, 0.020f, 0.055f}, {0.043f, 0.043f, 0.071f} }, // #050510 / #0B0B12
        {  5.5f, {0.055f, 0.051f, 0.094f}, {0.153f, 0.114f, 0.129f} }, // #0E0D18 / #271D21
        {  6.5f, {0.184f, 0.176f, 0.290f}, {0.510f, 0.318f, 0.224f} }, // #2F2D4A / #825139
        {  8.0f, {0.357f, 0.506f, 0.722f}, {0.647f, 0.600f, 0.482f} }, // #5B81B8 / #A5997B
        { 12.0f, {0.400f, 0.600f, 0.850f}, {0.706f, 0.725f, 0.678f} }, // #6699D9 / #B4B9AD
        { 16.0f, {0.357f, 0.506f, 0.722f}, {0.647f, 0.600f, 0.482f} }, // #5B81B8 / #A5997B
        { 18.0f, {0.286f, 0.208f, 0.310f}, {0.608f, 0.365f, 0.247f} }, // #49354F / #9B5D3F
        { 19.5f, {0.055f, 0.051f, 0.094f}, {0.153f, 0.114f, 0.129f} }, // #0E0D18 / #271D21
        { 24.0f, {0.020f, 0.020f, 0.055f}, {0.043f, 0.043f, 0.071f} }, // #050510 / #0B0B12
    };

    void update(float deltaTime) {
        timeOfDay += deltaTime * timeSpeed / 60.0f;
        if (timeOfDay >= 24.0f) timeOfDay -= 24.0f;
        if (timeOfDay < 0.0f) timeOfDay += 24.0f;

        // Lamp intensity: smooth fade
        // On at 19.0, off at 6.5
        if (timeOfDay >= 19.0f || timeOfDay < 6.5f) {
            lampIntensity = std::min(lampIntensity + deltaTime * 0.5f, 1.0f);
        } else {
            lampIntensity = std::max(lampIntensity - deltaTime * 0.5f, 0.0f);
        }

        // Star visibility
        if (timeOfDay >= 20.0f || timeOfDay < 5.5f) {
            starAlpha = std::min(starAlpha + deltaTime * 0.3f, 1.0f);
        } else {
            starAlpha = std::max(starAlpha - deltaTime * 0.3f, 0.0f);
        }

        // Update fog color from horizon keyframe
        fogColor = getHorizonColor();
    }

    void cycleSpeed() {
        speedMode = (speedMode + 1) % 4;
        float speeds[] = { 0.0f, 1.0f, 8.0f, 30.0f };
        timeSpeed = speeds[speedMode];
    }

    glm::vec3 getTopColor() const {
        return interpolateKeyframes(true);
    }

    glm::vec3 getHorizonColor() const {
        return interpolateKeyframes(false);
    }

    // Sun direction (rises in east, sets in west)
    // Sun angle: maps timeOfDay to an arc
    glm::vec3 getSunDirection() const {
        float angle = (timeOfDay - 6.0f) / 12.0f * glm::pi<float>(); // 6=horizon, 12=zenith, 18=set
        float sy = -sinf(angle);
        float sx = cosf(angle);
        return glm::normalize(glm::vec3(sx, sy, -0.2f));
    }

    glm::vec3 getSunPosition() const {
        return -getSunDirection() * 400.0f;
    }

    glm::vec3 getMoonDirection() const {
        // Moon opposite to sun, offset by 12 hours
        float angle = (timeOfDay - 18.0f) / 12.0f * glm::pi<float>();
        float sy = -sinf(angle);
        float sx = cosf(angle);
        return glm::normalize(glm::vec3(sx, sy, 0.3f));
    }

    glm::vec3 getMoonPosition() const {
        return -getMoonDirection() * 500.0f;
    }

    float getSunGlowStrength() const {
        // Strong glow at dawn/dusk, none at night
        if (timeOfDay > 5.0f && timeOfDay < 8.0f)
            return 1.5f * (1.0f - fabsf(timeOfDay - 6.5f) / 1.5f);
        if (timeOfDay > 17.0f && timeOfDay < 20.0f)
            return 1.5f * (1.0f - fabsf(timeOfDay - 18.5f) / 1.5f);
        if (timeOfDay >= 8.0f && timeOfDay <= 17.0f)
            return 0.5f;
        return 0.0f;
    }

    bool isDaytime() const {
        return timeOfDay >= 6.5f && timeOfDay < 19.0f;
    }

    float getNightFactor() const {
        if (timeOfDay >= 20.0f || timeOfDay < 5.5f) return 1.0f;
        if (timeOfDay >= 5.5f && timeOfDay < 6.5f) return 1.0f - (timeOfDay - 5.5f);
        if (timeOfDay >= 19.0f && timeOfDay < 20.0f) return timeOfDay - 19.0f;
        return 0.0f;
    }

    // Sun light intensity (for directional light strength)
    float getSunIntensity() const {
        if (timeOfDay < 5.5f || timeOfDay > 19.5f) return 0.0f;
        if (timeOfDay < 6.5f) return (timeOfDay - 5.5f);
        if (timeOfDay > 18.0f) return (19.5f - timeOfDay) / 1.5f;
        return 1.0f;
    }

    float getMoonIntensity() const {
        if (timeOfDay >= 6.5f && timeOfDay <= 19.0f) return 0.0f;
        if (timeOfDay < 5.5f || timeOfDay > 20.0f) return 0.25f;
        if (timeOfDay >= 19.0f && timeOfDay <= 20.0f) return (timeOfDay - 19.0f) * 0.25f;
        if (timeOfDay >= 5.5f && timeOfDay <= 6.5f) return (6.5f - timeOfDay) * 0.25f;
        return 0.0f;
    }

    std::string getSpeedLabel() const {
        switch (speedMode) {
            case 0: return "PAUSED";
            case 1: return "1x";
            case 2: return "8x";
            case 3: return "30x";
        }
        return "?";
    }

private:
    glm::vec3 interpolateKeyframes(bool isTop) const {
        for (int i = 0; i < NUM_KEYFRAMES - 1; i++) {
            if (timeOfDay >= keyframes[i].hour && timeOfDay <= keyframes[i + 1].hour) {
                float t = (timeOfDay - keyframes[i].hour) / (keyframes[i + 1].hour - keyframes[i].hour);
                if (isTop) return glm::mix(keyframes[i].top, keyframes[i + 1].top, t);
                else return glm::mix(keyframes[i].horizon, keyframes[i + 1].horizon, t);
            }
        }
        return isTop ? keyframes[0].top : keyframes[0].horizon;
    }
};

#endif
