#ifndef DIRECTIONALLIGHT_H
#define DIRECTIONALLIGHT_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include "Shader.h"

class DirectionalLight {
public:
    glm::vec3 direction;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    DirectionalLight() : direction(0.0f, -1.0f, 0.0f), ambient(0.0f), diffuse(0.0f), specular(0.0f) {}

    DirectionalLight(glm::vec3 dir, glm::vec3 amb, glm::vec3 diff, glm::vec3 spec)
        : direction(dir), ambient(amb), diffuse(diff), specular(spec) {}

    void setUpLight(Shader& shader) {
        shader.use();
        shader.setVec3("dirLight.direction", direction);
        shader.setVec3("dirLight.ambient",   ambientOn * ambient);
        shader.setVec3("dirLight.diffuse",   diffuseOn * diffuse);
        shader.setVec3("dirLight.specular",  specularOn * specular);
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
