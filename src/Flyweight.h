#ifndef FLYWEIGHT_H
#define FLYWEIGHT_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <unordered_map>
#include <vector>
#include "Shader.h"

// Shared (intrinsic) mesh data — loaded once, drawn many times
class MeshFlyweight {
public:
    GLuint VAO = 0, VBO = 0, EBO = 0;
    int vertexCount = 0;
    int indexCount = 0;
    bool useIndices = false;

    void initFromData(const std::vector<float>& vertices, const std::vector<unsigned int>& indices = {}) {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

        if (!indices.empty()) {
            useIndices = true;
            indexCount = (int)indices.size();
            glGenBuffers(1, &EBO);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
        }
        vertexCount = (int)vertices.size() / 8; // 8 floats per vertex

        int stride = 8 * sizeof(float);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glBindVertexArray(0);
    }

    void draw() const {
        glBindVertexArray(VAO);
        if (useIndices)
            glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
        else
            glDrawArrays(GL_TRIANGLES, 0, vertexCount);
        glBindVertexArray(0);
    }

    void cleanup() {
        if (VAO) glDeleteVertexArrays(1, &VAO);
        if (VBO) glDeleteBuffers(1, &VBO);
        if (EBO) glDeleteBuffers(1, &EBO);
    }
};

// Per-instance (extrinsic) state
struct InstanceData {
    glm::mat4 modelMatrix;
    glm::vec3 colorTint;
    bool useDarkTexture; // e.g., Block 11

    InstanceData(glm::mat4 mat = glm::mat4(1.0f),
                 glm::vec3 tint = glm::vec3(1.0f),
                 bool dark = false)
        : modelMatrix(mat), colorTint(tint), useDarkTexture(dark) {}
};

// Factory: creates and caches shared meshes
class FlyweightFactory {
public:
    std::unordered_map<std::string, MeshFlyweight*> pool;

    MeshFlyweight* get(const std::string& key) {
        auto it = pool.find(key);
        if (it != pool.end())
            return it->second;
        // Not found — create new (caller must init it)
        auto* fw = new MeshFlyweight();
        pool[key] = fw;
        return fw;
    }

    void cleanup() {
        for (auto& pair : pool) {
            pair.second->cleanup();
            delete pair.second;
        }
        pool.clear();
    }
};

#endif
