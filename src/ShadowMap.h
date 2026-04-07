#ifndef SHADOWMAP_H
#define SHADOWMAP_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// ================================================================
// ShadowMap: Sun-only directional shadow with a single 4096² depth FBO
// Per Section 5.3
// ================================================================
class ShadowMap {
public:
    static constexpr int SHADOW_SIZE = 4096;

    GLuint depthFBO = 0;
    GLuint depthTexture = 0;
    glm::mat4 lightSpaceMatrix = glm::mat4(1.0f);
    glm::mat4 lightView = glm::mat4(1.0f);
    glm::mat4 lightProjection = glm::mat4(1.0f);

    bool init() {
        glGenFramebuffers(1, &depthFBO);
        glGenTextures(1, &depthTexture);

        glBindTexture(GL_TEXTURE_2D, depthTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
                     SHADOW_SIZE, SHADOW_SIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

        glBindFramebuffer(GL_FRAMEBUFFER, depthFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);

        bool ok = (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return ok;
    }

    // Update the light space matrix based on current sun direction
    void updateLightSpace(const glm::vec3& sunDirection) {
        // The sun shines in the direction 'sunDirection'
        // The source position is the opposite of that direction, far away
        glm::vec3 lightPos = -sunDirection * 200.0f;
        glm::vec3 target(0.0f, 0.0f, 0.0f);
        glm::vec3 up(0.0f, 1.0f, 0.0f);

        // Avoid up-vector parallel to direction
        if (fabsf(glm::dot(glm::normalize(-sunDirection), up)) > 0.99f)
            up = glm::vec3(0.0f, 0.0f, 1.0f);

        lightView = glm::lookAt(lightPos, target, up);
        // Orthographic projection covering the entire camp
        float extent = 200.0f;
        lightProjection = glm::ortho(-extent, extent, -extent, extent, 1.0f, 500.0f);
        lightSpaceMatrix = lightProjection * lightView;
    }

    // Begin shadow render pass
    void beginShadowPass() const {
        glViewport(0, 0, SHADOW_SIZE, SHADOW_SIZE);
        glBindFramebuffer(GL_FRAMEBUFFER, depthFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        // Enable front-face culling to reduce shadow acne
        glCullFace(GL_FRONT);
    }

    // End shadow render pass and restore state
    void endShadowPass(int windowWidth, int windowHeight) const {
        glCullFace(GL_BACK);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, windowWidth, windowHeight);
    }

    // Bind shadow map texture for reading in the main pass
    void bindForReading(int textureUnit) const {
        glActiveTexture(GL_TEXTURE0 + textureUnit);
        glBindTexture(GL_TEXTURE_2D, depthTexture);
    }

    void cleanup() {
        if (depthFBO) glDeleteFramebuffers(1, &depthFBO);
        if (depthTexture) glDeleteTextures(1, &depthTexture);
    }
};

#endif
