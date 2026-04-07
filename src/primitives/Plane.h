#ifndef PLANE_H
#define PLANE_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Shader.h"

class Plane {
public:
    unsigned int VAO = 0, VBO = 0;

    void init() {
        // A unit quad on XZ plane at Y=0, from (0,0,0) to (1,0,1)
        // pos(3) + normal(3) + uv(2) = 8 floats
        float vertices[] = {
            0,0,0, 0,1,0, 0,0,
            1,0,0, 0,1,0, 1,0,
            1,0,1, 0,1,0, 1,1,

            1,0,1, 0,1,0, 1,1,
            0,0,1, 0,1,0, 0,1,
            0,0,0, 0,1,0, 0,0,
        };

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

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
        model = glm::scale(model, glm::vec3(scaleX, 1.0f, scaleZ));
        shader.setMat4("model", model);

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
    }

    void cleanup() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
    }
};

#endif
