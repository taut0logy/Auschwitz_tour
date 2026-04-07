#ifndef CYLINDER_H
#define CYLINDER_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <cmath>
#include "Shader.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

class Cylinder {
public:
    unsigned int VAO = 0, VBO = 0, EBO = 0;
    int indexCount = 0;

    void init(int sectors = 36) {
        std::vector<float> vertices;
        std::vector<unsigned int> indices;

        float radius = 0.5f;
        float height = 1.0f;
        float sectorStep = 2.0f * (float)M_PI / sectors;

        // Side vertices: bottom + top rings
        for (int i = 0; i <= sectors; i++) {
            float angle = i * sectorStep;
            float x = radius * cosf(angle);
            float z = radius * sinf(angle);
            float nx = cosf(angle);
            float nz = sinf(angle);
            float u = (float)i / (float)sectors;

            // Bottom ring
            vertices.insert(vertices.end(), {x+0.5f, 0.0f, z+0.5f, nx, 0.0f, nz, u, 0.0f});
            // Top ring
            vertices.insert(vertices.end(), {x+0.5f, height, z+0.5f, nx, 0.0f, nz, u, 1.0f});
        }

        for (int i = 0; i < sectors; i++) {
            int b = i*2, t = i*2+1, nb = (i+1)*2, nt = (i+1)*2+1;
            indices.insert(indices.end(), {(unsigned)b, (unsigned)nb, (unsigned)t,
                                           (unsigned)t, (unsigned)nb, (unsigned)nt});
        }

        // Bottom cap
        int bcIdx = (int)vertices.size() / 8;
        vertices.insert(vertices.end(), {0.5f, 0.0f, 0.5f, 0.0f, -1.0f, 0.0f, 0.5f, 0.5f});
        int brStart = (int)vertices.size() / 8;
        for (int i = 0; i <= sectors; i++) {
            float angle = i * sectorStep;
            float x = radius * cosf(angle) + 0.5f;
            float z = radius * sinf(angle) + 0.5f;
            vertices.insert(vertices.end(), {x, 0.0f, z, 0.0f, -1.0f, 0.0f, x, z});
        }
        for (int i = 0; i < sectors; i++)
            indices.insert(indices.end(), {(unsigned)bcIdx, (unsigned)(brStart+i+1), (unsigned)(brStart+i)});

        // Top cap
        int tcIdx = (int)vertices.size() / 8;
        vertices.insert(vertices.end(), {0.5f, height, 0.5f, 0.0f, 1.0f, 0.0f, 0.5f, 0.5f});
        int trStart = (int)vertices.size() / 8;
        for (int i = 0; i <= sectors; i++) {
            float angle = i * sectorStep;
            float x = radius * cosf(angle) + 0.5f;
            float z = radius * sinf(angle) + 0.5f;
            vertices.insert(vertices.end(), {x, height, z, 0.0f, 1.0f, 0.0f, x, z});
        }
        for (int i = 0; i < sectors; i++)
            indices.insert(indices.end(), {(unsigned)tcIdx, (unsigned)(trStart+i), (unsigned)(trStart+i+1)});

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

    void cleanup() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
    }
};

#endif
