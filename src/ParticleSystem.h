#ifndef PARTICLESYSTEM_H
#define PARTICLESYSTEM_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <algorithm>
#include "Shader.h"
#include "Texture.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ================================================================
// Koch Snowflake Fractal Texture Generator
// ================================================================
class KochSnowflake {
public:
    static unsigned int generateTexture(int size = 64, int iterations = 3) {
        // Allocate RGBA buffer
        std::vector<unsigned char> pixels(size * size * 4, 0);

        // Generate Koch snowflake vertices
        std::vector<glm::vec2> points;

        // Start with equilateral triangle
        float cx = size / 2.0f, cy = size / 2.0f;
        float r = size * 0.4f;
        for (int i = 0; i < 3; i++) {
            float angle = (float)M_PI / 2.0f + i * 2.0f * (float)M_PI / 3.0f;
            points.push_back(glm::vec2(cx + r * cosf(angle), cy + r * sinf(angle)));
        }

        // Apply Koch subdivision
        for (int iter = 0; iter < iterations; iter++) {
            std::vector<glm::vec2> newPoints;
            for (size_t i = 0; i < points.size(); i++) {
                glm::vec2 A = points[i];
                glm::vec2 B = points[(i + 1) % points.size()];
                glm::vec2 d = B - A;
                glm::vec2 P1 = A + d / 3.0f;
                glm::vec2 P2 = A + d * (2.0f / 3.0f);

                // Peak point: rotate middle third outward by 60 degrees
                glm::vec2 mid = (P1 + P2) * 0.5f;
                glm::vec2 perp = glm::vec2(-(P2.y - P1.y), P2.x - P1.x);
                float len = glm::length(perp);
                if (len > 0.001f) perp = perp / len;
                glm::vec2 peak = mid + perp * (glm::length(P2 - P1) * sqrtf(3.0f) / 2.0f);

                newPoints.push_back(A);
                newPoints.push_back(P1);
                newPoints.push_back(peak);
                newPoints.push_back(P2);
            }
            points = newPoints;
        }

        // Rasterize lines into pixel buffer (Bresenham-style)
        for (size_t i = 0; i < points.size(); i++) {
            glm::vec2 A = points[i];
            glm::vec2 B = points[(i + 1) % points.size()];
            drawLine(pixels, size, (int)A.x, (int)A.y, (int)B.x, (int)B.y);
        }

        // Fill the interior with a simple flood fill from center
        floodFill(pixels, size, size/2, size/2);

        // Upload to OpenGL texture
        return createTextureFromData(pixels.data(), size, size, GL_CLAMP_TO_EDGE);
    }

private:
    static void setPixel(std::vector<unsigned char>& pixels, int size, int x, int y) {
        if (x >= 0 && x < size && y >= 0 && y < size) {
            int idx = (y * size + x) * 4;
            pixels[idx] = 255;
            pixels[idx+1] = 255;
            pixels[idx+2] = 255;
            pixels[idx+3] = 255;
        }
    }

    static bool getPixel(std::vector<unsigned char>& pixels, int size, int x, int y) {
        if (x < 0 || x >= size || y < 0 || y >= size) return true;
        return pixels[(y * size + x) * 4 + 3] > 0;
    }

    static void drawLine(std::vector<unsigned char>& pixels, int size, int x0, int y0, int x1, int y1) {
        int dx = abs(x1-x0), sx = x0<x1 ? 1 : -1;
        int dy = -abs(y1-y0), sy = y0<y1 ? 1 : -1;
        int err = dx+dy;
        while (true) {
            setPixel(pixels, size, x0, y0);
            // Also draw neighboring pixels for thickness
            setPixel(pixels, size, x0+1, y0);
            setPixel(pixels, size, x0, y0+1);
            if (x0 == x1 && y0 == y1) break;
            int e2 = 2*err;
            if (e2 >= dy) { err += dy; x0 += sx; }
            if (e2 <= dx) { err += dx; y0 += sy; }
        }
    }

    static void floodFill(std::vector<unsigned char>& pixels, int size, int startX, int startY) {
        if (getPixel(pixels, size, startX, startY)) return;
        std::vector<glm::ivec2> stack;
        stack.push_back(glm::ivec2(startX, startY));
        while (!stack.empty()) {
            glm::ivec2 p = stack.back();
            stack.pop_back();
            if (p.x < 0 || p.x >= size || p.y < 0 || p.y >= size) continue;
            if (getPixel(pixels, size, p.x, p.y)) continue;
            setPixel(pixels, size, p.x, p.y);
            stack.push_back(glm::ivec2(p.x+1, p.y));
            stack.push_back(glm::ivec2(p.x-1, p.y));
            stack.push_back(glm::ivec2(p.x, p.y+1));
            stack.push_back(glm::ivec2(p.x, p.y-1));
        }
    }
};

// ================================================================
// Particle System: Snowflakes + Chimney Smoke
// ================================================================
struct Particle {
    glm::vec3 position;
    float speed;
    float spin;
    float size;
    float drift;
    float alpha;
};

class ParticleSystem {
public:
    // Snowflake system
    std::vector<Particle> snowflakes;
    unsigned int snowflakeTexture = 0;
    GLuint snowVAO = 0, snowVBO = 0;

    // Smoke system
    std::vector<Particle> smokeParticles;
    GLuint smokeVAO = 0, smokeVBO = 0;

    glm::vec3 chimneyTop = glm::vec3(-125.0f, 12.0f, -32.0f);

    void init(int numSnowflakes = 500, int numSmoke = 50) {
        // Generate Koch snowflake texture procedurally
        snowflakeTexture = KochSnowflake::generateTexture(64, 3);

        // Initialize snowflakes
        snowflakes.resize(numSnowflakes);
        for (auto& s : snowflakes) {
            resetSnowflake(s);
            s.position.y = ((float)rand() / RAND_MAX) * 40.0f + 5.0f; // Spread vertically
        }

        // Initialize smoke
        smokeParticles.resize(numSmoke);
        for (auto& p : smokeParticles) {
            resetSmoke(p);
            p.position.y = chimneyTop.y + ((float)rand() / RAND_MAX) * 5.0f;
        }

        // Create VAOs
        glGenVertexArrays(1, &snowVAO);
        glGenBuffers(1, &snowVBO);
        glGenVertexArrays(1, &smokeVAO);
        glGenBuffers(1, &smokeVBO);
    }

    void update(float deltaTime, float timeOfDay) {
        // Update snowflakes
        float snowAlpha = 0.0f;
        if (timeOfDay > 19.0f)
            snowAlpha = std::clamp((timeOfDay - 19.0f) / 2.0f, 0.0f, 1.0f);
        else if (timeOfDay < 6.0f)
            snowAlpha = std::clamp((6.0f - timeOfDay) / 1.0f, 0.0f, 1.0f);

        for (auto& s : snowflakes) {
            s.position.y -= s.speed * deltaTime;
            s.position.x += sinf(s.drift) * 0.3f * deltaTime;
            s.drift += deltaTime * 0.5f;
            s.spin += deltaTime * 2.0f;
            s.alpha = snowAlpha;

            if (s.position.y < 0.0f) resetSnowflake(s);
        }

        // Update smoke
        for (auto& p : smokeParticles) {
            p.position.y += p.speed * deltaTime;
            p.position.x += sinf(p.drift) * 0.1f * deltaTime;
            p.drift += deltaTime;
            p.size += deltaTime * 0.2f;
            p.alpha -= deltaTime * 0.15f;

            if (p.alpha <= 0.0f || p.position.y > chimneyTop.y + 10.0f) resetSmoke(p);
        }
    }

    void renderSnowflakes(Shader& shader, const glm::mat4& view, const glm::mat4& projection) {
        if (snowflakes.empty() || snowflakes[0].alpha <= 0.01f) return;

        shader.use();
        shader.setMat4("view", view);
        shader.setMat4("projection", projection);
        shader.setBool("useParticleTex", true);
        shader.setVec3("particleColor", glm::vec3(1.0f));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, snowflakeTexture);
        shader.setInt("particleTex", 0);

        // Build billboard quads
        glm::vec3 camRight = glm::vec3(view[0][0], view[1][0], view[2][0]);
        glm::vec3 camUp = glm::vec3(view[0][1], view[1][1], view[2][1]);

        std::vector<float> verts;
        for (auto& s : snowflakes) {
            if (s.alpha <= 0.01f) continue;
            float hs = s.size * 0.5f;
            glm::vec3 r = camRight * hs;
            glm::vec3 u = camUp * hs;
            glm::vec3 p = s.position;

            // 6 vertices for a quad (2 triangles)
            auto addVert = [&](glm::vec3 pos, float texU, float texV) {
                verts.insert(verts.end(), {pos.x, pos.y, pos.z, texU, texV, s.alpha});
            };
            addVert(p - r - u, 0, 0);
            addVert(p + r - u, 1, 0);
            addVert(p + r + u, 1, 1);
            addVert(p + r + u, 1, 1);
            addVert(p - r + u, 0, 1);
            addVert(p - r - u, 0, 0);
        }

        if (verts.empty()) return;

        glBindVertexArray(snowVAO);
        glBindBuffer(GL_ARRAY_BUFFER, snowVBO);
        glBufferData(GL_ARRAY_BUFFER, verts.size()*sizeof(float), verts.data(), GL_DYNAMIC_DRAW);

        // pos(3) + uv(2) + alpha(1) = 6 floats
        int stride = 6 * sizeof(float);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(3*sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, stride, (void*)(5*sizeof(float)));
        glEnableVertexAttribArray(2);

        glDrawArrays(GL_TRIANGLES, 0, (int)verts.size() / 6);
        glBindVertexArray(0);
    }

    void renderSmoke(Shader& shader, const glm::mat4& view, const glm::mat4& projection) {
        shader.use();
        shader.setMat4("view", view);
        shader.setMat4("projection", projection);
        shader.setBool("useParticleTex", false);
        shader.setVec3("particleColor", glm::vec3(0.5f, 0.5f, 0.5f));

        glm::vec3 camRight = glm::vec3(view[0][0], view[1][0], view[2][0]);
        glm::vec3 camUp = glm::vec3(view[0][1], view[1][1], view[2][1]);

        std::vector<float> verts;
        for (auto& p : smokeParticles) {
            if (p.alpha <= 0.01f) continue;
            float hs = p.size * 0.5f;
            glm::vec3 r = camRight * hs;
            glm::vec3 u = camUp * hs;
            glm::vec3 pos = p.position;

            auto addVert = [&](glm::vec3 v, float texU, float texV) {
                verts.insert(verts.end(), {v.x, v.y, v.z, texU, texV, p.alpha});
            };
            addVert(pos-r-u,0,0); addVert(pos+r-u,1,0); addVert(pos+r+u,1,1);
            addVert(pos+r+u,1,1); addVert(pos-r+u,0,1); addVert(pos-r-u,0,0);
        }

        if (verts.empty()) return;

        glBindVertexArray(smokeVAO);
        glBindBuffer(GL_ARRAY_BUFFER, smokeVBO);
        glBufferData(GL_ARRAY_BUFFER, verts.size()*sizeof(float), verts.data(), GL_DYNAMIC_DRAW);

        int stride = 6 * sizeof(float);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(3*sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, stride, (void*)(5*sizeof(float)));
        glEnableVertexAttribArray(2);

        glDrawArrays(GL_TRIANGLES, 0, (int)verts.size() / 6);
        glBindVertexArray(0);
    }

    void cleanup() {
        if (snowVAO) { glDeleteVertexArrays(1, &snowVAO); glDeleteBuffers(1, &snowVBO); }
        if (smokeVAO) { glDeleteVertexArrays(1, &smokeVAO); glDeleteBuffers(1, &smokeVBO); }
        if (snowflakeTexture) glDeleteTextures(1, &snowflakeTexture);
    }

private:
    void resetSnowflake(Particle& s) {
        s.position = glm::vec3(
            ((float)rand()/RAND_MAX - 0.5f) * 260.0f,  // X: -130..130
            20.0f + ((float)rand()/RAND_MAX) * 20.0f,    // Y: above scene
            ((float)rand()/RAND_MAX - 0.5f) * 120.0f     // Z: -60..60
        );
        s.speed = 0.5f + ((float)rand()/RAND_MAX) * 1.5f;
        s.spin = ((float)rand()/RAND_MAX) * 6.28f;
        s.size = 0.15f + ((float)rand()/RAND_MAX) * 0.25f;
        s.drift = ((float)rand()/RAND_MAX) * 6.28f;
        s.alpha = 0.0f;
    }

    void resetSmoke(Particle& p) {
        p.position = chimneyTop + glm::vec3(
            ((float)rand()/RAND_MAX - 0.5f) * 0.3f,
            0.0f,
            ((float)rand()/RAND_MAX - 0.5f) * 0.3f
        );
        p.speed = 0.8f + ((float)rand()/RAND_MAX) * 0.5f;
        p.spin = 0.0f;
        p.size = 0.3f;
        p.drift = ((float)rand()/RAND_MAX) * 6.28f;
        p.alpha = 0.6f + ((float)rand()/RAND_MAX) * 0.3f;
    }
};

#endif
