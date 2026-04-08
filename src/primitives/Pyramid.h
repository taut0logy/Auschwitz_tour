#ifndef PYRAMID_H
#define PYRAMID_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include "Shader.h"

class Pyramid {
public:
    GLuint vao = 0, vbo = 0, ebo = 0;
    int vertexCount = 0;

    void init() {
        // Unit pyramid centered on origin: base y=0, apex y=1.
        const float b = 0.5f;
        const glm::vec3 p0(-b, 0.0f, -b); // back-left
        const glm::vec3 p1( b, 0.0f, -b); // back-right
        const glm::vec3 p2( b, 0.0f,  b); // front-right
        const glm::vec3 p3(-b, 0.0f,  b); // front-left
        const glm::vec3 apex(0.0f, 1.0f, 0.0f);

        std::vector<float> vertices;
        vertices.reserve(18 * 8);

        auto pushTri = [&](const glm::vec3& a, const glm::vec3& c, const glm::vec3& d,
                           const glm::vec2& uva, const glm::vec2& uvc, const glm::vec2& uvd) {
            glm::vec3 n = glm::normalize(glm::cross(c - a, d - a));
            vertices.insert(vertices.end(), { a.x, a.y, a.z, n.x, n.y, n.z, uva.x, uva.y });
            vertices.insert(vertices.end(), { c.x, c.y, c.z, n.x, n.y, n.z, uvc.x, uvc.y });
            vertices.insert(vertices.end(), { d.x, d.y, d.z, n.x, n.y, n.z, uvd.x, uvd.y });
        };

        // Side faces (outward winding).
        pushTri(apex, p2, p3, glm::vec2(0.5f, 1.0f), glm::vec2(1.0f, 0.0f), glm::vec2(0.0f, 0.0f)); // front
        pushTri(apex, p1, p2, glm::vec2(0.5f, 1.0f), glm::vec2(1.0f, 0.0f), glm::vec2(0.0f, 0.0f)); // right
        pushTri(apex, p0, p1, glm::vec2(0.5f, 1.0f), glm::vec2(1.0f, 0.0f), glm::vec2(0.0f, 0.0f)); // back
        pushTri(apex, p3, p0, glm::vec2(0.5f, 1.0f), glm::vec2(1.0f, 0.0f), glm::vec2(0.0f, 0.0f)); // left

        // Base (two triangles), normal points downward.
        const glm::vec3 down(0.0f, -1.0f, 0.0f);
        auto pushBase = [&](const glm::vec3& a, const glm::vec3& c, const glm::vec3& d,
                            const glm::vec2& uva, const glm::vec2& uvc, const glm::vec2& uvd) {
            vertices.insert(vertices.end(), { a.x, a.y, a.z, down.x, down.y, down.z, uva.x, uva.y });
            vertices.insert(vertices.end(), { c.x, c.y, c.z, down.x, down.y, down.z, uvc.x, uvc.y });
            vertices.insert(vertices.end(), { d.x, d.y, d.z, down.x, down.y, down.z, uvd.x, uvd.y });
        };
        pushBase(p0, p2, p1, glm::vec2(0.0f, 1.0f), glm::vec2(1.0f, 0.0f), glm::vec2(1.0f, 1.0f));
        pushBase(p0, p3, p2, glm::vec2(0.0f, 1.0f), glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 0.0f));

        vertexCount = (int)(vertices.size() / 8);

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

        int stride = 8 * sizeof(float);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
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

        // Position at base (y=0), scale will stretch height
        glm::mat4 model = glm::translate(parentTrans, glm::vec3(posX, posY, posZ));
        model = glm::scale(model, glm::vec3(scaleX, scaleY, scaleZ));
        shader.setMat4("model", model);

        if (vao) {
            glBindVertexArray(vao);
            glDrawArrays(GL_TRIANGLES, 0, vertexCount);
            glBindVertexArray(0);
        }
    }

    void cleanup() {
        if (vao) { glDeleteVertexArrays(1, &vao); vao = 0; }
        if (vbo) { glDeleteBuffers(1, &vbo); vbo = 0; }
    }
};

#endif