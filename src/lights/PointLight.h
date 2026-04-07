#ifndef POINTLIGHT_H
#define POINTLIGHT_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include "Shader.h"

class PointLight {
public:
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    float k_c, k_l, k_q;
    int lightNumber;

    PointLight() : lightNumber(0), position(0.0f), ambient(0.0f), diffuse(0.0f),
                   specular(0.0f), k_c(1.0f), k_l(0.09f), k_q(0.032f) {}

    PointLight(int num, glm::vec3 pos, glm::vec3 amb, glm::vec3 diff, glm::vec3 spec,
               float constant = 1.0f, float linear = 0.09f, float quadratic = 0.032f)
        : lightNumber(num), position(pos), ambient(amb), diffuse(diff), specular(spec),
          k_c(constant), k_l(linear), k_q(quadratic) {}

    void setUpLight(Shader& shader) {
        shader.use();
        std::string base = "pointLights[" + std::to_string(lightNumber) + "].";
        shader.setVec3(base + "position", position);
        shader.setVec3(base + "ambient",  ambientOn * ambient);
        shader.setVec3(base + "diffuse",  diffuseOn * diffuse);
        shader.setVec3(base + "specular", specularOn * specular);
        shader.setFloat(base + "k_c", k_c);
        shader.setFloat(base + "k_l", k_l);
        shader.setFloat(base + "k_q", k_q);
    }

    void turnOff() { ambientOn = diffuseOn = specularOn = 0.0f; }
    void turnOn()  { ambientOn = diffuseOn = specularOn = 1.0f; }
    void setAmbient(bool on)  { ambientOn  = on ? 1.0f : 0.0f; }
    void setDiffuse(bool on)  { diffuseOn  = on ? 1.0f : 0.0f; }
    void setSpecular(bool on) { specularOn = on ? 1.0f : 0.0f; }

private:
    float ambientOn = 1.0f, diffuseOn = 1.0f, specularOn = 1.0f;
};

#endif
