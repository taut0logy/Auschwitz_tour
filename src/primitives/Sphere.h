#ifndef SPHERE_H
#define SPHERE_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <cmath>
#include "Shader.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

class Sphere {
public:
    unsigned int VAO = 0, VBO = 0, EBO = 0;
    int indexCount = 0;

    void init(int sectors = 36, int stacks = 18) {
        std::vector<float> vertices;
        std::vector<unsigned int> indices;

        float radius = 0.5f;
        float sectorStep = 2.0f * (float)M_PI / sectors;
        float stackStep = (float)M_PI / stacks;

        for (int i = 0; i <= stacks; i++) {
            float stackAngle = (float)M_PI / 2.0f - i * stackStep;
            float xy = radius * cosf(stackAngle);
            float y  = radius * sinf(stackAngle);

            for (int j = 0; j <= sectors; j++) {
                float sectorAngle = j * sectorStep;
                float x = xy * cosf(sectorAngle);
                float z = xy * sinf(sectorAngle);

                float nx = cosf(stackAngle)*cosf(sectorAngle);
                float ny = sinf(stackAngle);
                float nz = cosf(stackAngle)*sinf(sectorAngle);
                float u  = (float)j / sectors;
                float v  = (float)i / stacks;

                vertices.insert(vertices.end(), {x+0.5f, y+0.5f, z+0.5f, nx, ny, nz, u, v});
            }
        }

        for (int i = 0; i < stacks; i++) {
            int k1 = i * (sectors + 1);
            int k2 = k1 + sectors + 1;
            for (int j = 0; j < sectors; j++, k1++, k2++) {
                if (i != 0)
                    indices.insert(indices.end(), {(unsigned)k1, (unsigned)k2, (unsigned)(k1+1)});
                if (i != stacks - 1)
                    indices.insert(indices.end(), {(unsigned)(k1+1), (unsigned)k2, (unsigned)(k2+1)});
            }
        }

        indexCount = (int)indices.size();

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size()*sizeof(float), vertices.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size()*sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

        int stride = 8 * sizeof(float);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3*sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6*sizeof(float)));
        glEnableVertexAttribArray(2);
        glBindVertexArray(0);
    }

    void draw(Shader& shader, glm::mat4 parentTrans,
              float posX, float posY, float posZ,
              float scaleX, float scaleY, float scaleZ,
              glm::vec3 color, float shininess = 32.0f) const
    {
        shader.use();
        shader.setVec3("material.ambient",  color * 0.3f);
        shader.setVec3("material.diffuse",  color);
        shader.setVec3("material.specular", glm::vec3(0.5f));
        shader.setVec3("material.emissive", glm::vec3(0.0f));
        shader.setFloat("material.shininess", shininess);

        glm::mat4 model = glm::translate(parentTrans, glm::vec3(posX, posY, posZ));
        model = glm::scale(model, glm::vec3(scaleX, scaleY, scaleZ));
        shader.setMat4("model", model);

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    // Unlit emissive variant (for sun/moon)
    void drawEmissive(Shader& shader, glm::mat4 parentTrans,
                      float posX, float posY, float posZ,
                      float scaleX, float scaleY, float scaleZ,
                      glm::vec3 color) const
    {
        shader.use();
        shader.setVec3("objectColor", color);
        glm::mat4 model = glm::translate(parentTrans, glm::vec3(posX, posY, posZ));
        model = glm::scale(model, glm::vec3(scaleX, scaleY, scaleZ));
        shader.setMat4("model", model);

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    void cleanup() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
    }
};

#endif
