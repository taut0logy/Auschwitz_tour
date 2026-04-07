#ifndef LSYSTEMTREE_H
#define LSYSTEMTREE_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <vector>
#include <stack>
#include <string>
#include <cmath>
#include "Shader.h"

// ================================================================
// LSystemTree: Fractal L-system trees (poplar style, 4 iterations)
// Per Section 10.1
// Rule: F[+F]F[-F]F
// Angle: 25.7°, length shrink 0.6, branch radius shrink 0.65
// ================================================================
class LSystemTree {
public:
    struct TreeInstance {
        glm::vec3 position;
        float scale;
    };

    GLuint branchVAO = 0, branchVBO = 0;
    int branchVertCount = 0;

    GLuint leafVAO = 0, leafVBO = 0;
    int leafVertCount = 0;

    std::vector<TreeInstance> trees;

    void init() {
        std::string axiom = "F";
        std::string rules = "F[+F]F[-F]F";
        int iterations = 4;  // v2: 4 iterations

        // Expand L-system
        std::string sentence = axiom;
        for (int i = 0; i < iterations; i++) {
            std::string next;
            for (char c : sentence) {
                if (c == 'F') next += rules;
                else next += c;
            }
            sentence = next;
        }

        // Generate geometry from sentence using turtle graphics
        generateGeometry(sentence);
        placeTrees();
    }

    void renderBranches(Shader& shader, const glm::mat4& I,
                        unsigned int texBark) const
    {
        if (branchVertCount <= 0) return;

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texBark);
        shader.setInt("texture1", 0);
        shader.setBool("useTexture", texBark != 0);
        shader.setFloat("texRepeat", 1.0f);
        shader.setVec3("material.ambient", glm::vec3(0.08f, 0.05f, 0.03f));
        shader.setVec3("material.diffuse", glm::vec3(0.38f, 0.28f, 0.18f));
        shader.setVec3("material.specular", glm::vec3(0.05f));
        shader.setVec3("material.emissive", glm::vec3(0.0f));
        shader.setFloat("material.shininess", 4.0f);

        for (const auto& tree : trees) {
            glm::mat4 model = glm::translate(I, tree.position);
            model = glm::scale(model, glm::vec3(tree.scale));
            shader.setMat4("model", model);
            glBindVertexArray(branchVAO);
            glDrawArrays(GL_TRIANGLES, 0, branchVertCount);
            glBindVertexArray(0);
        }
        shader.setBool("useTexture", false);
    }

    void renderLeaves(Shader& alphaShader, const glm::mat4& I,
                      unsigned int texLeaf) const
    {
        if (leafVertCount <= 0) return;

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texLeaf);
        alphaShader.setInt("texture1", 0);
        alphaShader.setBool("useTexture", texLeaf != 0);
        alphaShader.setFloat("texRepeat", 1.0f);
        alphaShader.setVec3("material.ambient", glm::vec3(0.03f, 0.06f, 0.02f));
        alphaShader.setVec3("material.diffuse", glm::vec3(0.2f, 0.4f, 0.15f));
        alphaShader.setVec3("material.specular", glm::vec3(0.02f));
        alphaShader.setVec3("material.emissive", glm::vec3(0.0f));
        alphaShader.setFloat("material.shininess", 2.0f);

        for (const auto& tree : trees) {
            glm::mat4 model = glm::translate(I, tree.position);
            model = glm::scale(model, glm::vec3(tree.scale));
            alphaShader.setMat4("model", model);
            glBindVertexArray(leafVAO);
            glDrawArrays(GL_TRIANGLES, 0, leafVertCount);
            glBindVertexArray(0);
        }
        alphaShader.setBool("useTexture", false);
    }

    void cleanup() {
        if (branchVAO) { glDeleteVertexArrays(1, &branchVAO); glDeleteBuffers(1, &branchVBO); }
        if (leafVAO) { glDeleteVertexArrays(1, &leafVAO); glDeleteBuffers(1, &leafVBO); }
    }

private:
    static constexpr float ANGLE = 25.7f;
    static constexpr float INIT_LENGTH = 2.0f;
    static constexpr float INIT_RADIUS = 0.12f;
    static constexpr float LENGTH_SHRINK = 0.6f;
    static constexpr float RADIUS_SHRINK = 0.65f;

    struct TurtleState {
        glm::vec3 pos;
        glm::vec3 dir;
        float length;
        float radius;
        int depth;
    };

    void generateGeometry(const std::string& sentence) {
        std::vector<float> branchVerts;
        std::vector<float> leafVerts;

        TurtleState turtle;
        turtle.pos = glm::vec3(0.0f, 0.0f, 0.0f);
        turtle.dir = glm::vec3(0.0f, 1.0f, 0.0f);
        turtle.length = INIT_LENGTH;
        turtle.radius = INIT_RADIUS;
        turtle.depth = 0;

        std::stack<TurtleState> stateStack;

        for (char c : sentence) {
            if (c == 'F') {
                // Draw a branch segment
                glm::vec3 endPos = turtle.pos + turtle.dir * turtle.length;
                addCylinder(branchVerts, turtle.pos, endPos, turtle.radius);
                turtle.pos = endPos;
                turtle.length *= LENGTH_SHRINK;
                turtle.radius *= RADIUS_SHRINK;
                turtle.depth++;
            }
            else if (c == '+') {
                // Rotate CW around Z axis
                float rad = glm::radians(ANGLE);
                glm::vec3 axis(sinf(turtle.depth * 0.7f), 0, cosf(turtle.depth * 0.7f));
                glm::mat4 rot = glm::rotate(glm::mat4(1.0f), rad, axis);
                turtle.dir = glm::normalize(glm::vec3(rot * glm::vec4(turtle.dir, 0.0f)));
            }
            else if (c == '-') {
                float rad = glm::radians(-ANGLE);
                glm::vec3 axis(sinf(turtle.depth * 0.7f), 0, cosf(turtle.depth * 0.7f));
                glm::mat4 rot = glm::rotate(glm::mat4(1.0f), rad, axis);
                turtle.dir = glm::normalize(glm::vec3(rot * glm::vec4(turtle.dir, 0.0f)));
            }
            else if (c == '[') {
                stateStack.push(turtle);
                // Also add leaf at branch tips
                if (turtle.depth > 3) {
                    addLeafQuad(leafVerts, turtle.pos, turtle.dir);
                }
            }
            else if (c == ']') {
                if (!stateStack.empty()) {
                    turtle = stateStack.top();
                    stateStack.pop();
                }
            }
        }

        // Upload branches
        branchVertCount = (int)branchVerts.size() / 8;
        if (branchVertCount > 0) {
            glGenVertexArrays(1, &branchVAO);
            glGenBuffers(1, &branchVBO);
            glBindVertexArray(branchVAO);
            glBindBuffer(GL_ARRAY_BUFFER, branchVBO);
            glBufferData(GL_ARRAY_BUFFER, branchVerts.size() * sizeof(float), branchVerts.data(), GL_STATIC_DRAW);
            int stride = 8 * sizeof(float);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
            glEnableVertexAttribArray(2);
            glBindVertexArray(0);
        }

        // Upload leaves
        leafVertCount = (int)leafVerts.size() / 8;
        if (leafVertCount > 0) {
            glGenVertexArrays(1, &leafVAO);
            glGenBuffers(1, &leafVBO);
            glBindVertexArray(leafVAO);
            glBindBuffer(GL_ARRAY_BUFFER, leafVBO);
            glBufferData(GL_ARRAY_BUFFER, leafVerts.size() * sizeof(float), leafVerts.data(), GL_STATIC_DRAW);
            int stride = 8 * sizeof(float);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
            glEnableVertexAttribArray(2);
            glBindVertexArray(0);
        }
    }

    void addCylinder(std::vector<float>& verts,
                     const glm::vec3& base, const glm::vec3& tip, float radius) const
    {
        int sides = 6;
        glm::vec3 dir = glm::normalize(tip - base);
        glm::vec3 perp;
        if (fabsf(dir.y) < 0.99f)
            perp = glm::normalize(glm::cross(dir, glm::vec3(0, 1, 0)));
        else
            perp = glm::normalize(glm::cross(dir, glm::vec3(1, 0, 0)));
        glm::vec3 perp2 = glm::normalize(glm::cross(dir, perp));

        for (int i = 0; i < sides; i++) {
            float a0 = (float)i / sides * 2.0f * glm::pi<float>();
            float a1 = (float)(i + 1) / sides * 2.0f * glm::pi<float>();
            glm::vec3 n0 = perp * cosf(a0) + perp2 * sinf(a0);
            glm::vec3 n1 = perp * cosf(a1) + perp2 * sinf(a1);
            glm::vec3 b0 = base + n0 * radius;
            glm::vec3 b1 = base + n1 * radius;
            glm::vec3 t0 = tip + n0 * radius * 0.7f;
            glm::vec3 t1 = tip + n1 * radius * 0.7f;

            // Triangle 1
            verts.insert(verts.end(), {b0.x,b0.y,b0.z, n0.x,n0.y,n0.z, 0,0});
            verts.insert(verts.end(), {b1.x,b1.y,b1.z, n1.x,n1.y,n1.z, 1,0});
            verts.insert(verts.end(), {t0.x,t0.y,t0.z, n0.x,n0.y,n0.z, 0,1});
            // Triangle 2
            verts.insert(verts.end(), {t0.x,t0.y,t0.z, n0.x,n0.y,n0.z, 0,1});
            verts.insert(verts.end(), {b1.x,b1.y,b1.z, n1.x,n1.y,n1.z, 1,0});
            verts.insert(verts.end(), {t1.x,t1.y,t1.z, n1.x,n1.y,n1.z, 1,1});
        }
    }

    void addLeafQuad(std::vector<float>& verts,
                     const glm::vec3& pos, const glm::vec3& dir) const
    {
        float size = 1.2f; // Much larger leaf cluster billboard
        glm::vec3 n(0, 1, 0); // Upward normal for lighting
        
        // Plane 1 (XZ aligned)
        glm::vec3 right1(1, 0, 0);
        glm::vec3 p0_1 = pos - right1 * size;
        glm::vec3 p1_1 = pos + right1 * size;
        glm::vec3 p2_1 = pos + right1 * size + glm::vec3(0, size*1.5f, 0);
        glm::vec3 p3_1 = pos - right1 * size + glm::vec3(0, size*1.5f, 0);

        verts.insert(verts.end(), {p0_1.x,p0_1.y,p0_1.z, n.x,n.y,n.z, 0,0});
        verts.insert(verts.end(), {p1_1.x,p1_1.y,p1_1.z, n.x,n.y,n.z, 1,0});
        verts.insert(verts.end(), {p2_1.x,p2_1.y,p2_1.z, n.x,n.y,n.z, 1,1});
        verts.insert(verts.end(), {p2_1.x,p2_1.y,p2_1.z, n.x,n.y,n.z, 1,1});
        verts.insert(verts.end(), {p3_1.x,p3_1.y,p3_1.z, n.x,n.y,n.z, 0,1});
        verts.insert(verts.end(), {p0_1.x,p0_1.y,p0_1.z, n.x,n.y,n.z, 0,0});

        // Plane 2 (YZ aligned)
        glm::vec3 right2(0, 0, 1);
        glm::vec3 p0_2 = pos - right2 * size;
        glm::vec3 p1_2 = pos + right2 * size;
        glm::vec3 p2_2 = pos + right2 * size + glm::vec3(0, size*1.5f, 0);
        glm::vec3 p3_2 = pos - right2 * size + glm::vec3(0, size*1.5f, 0);

        verts.insert(verts.end(), {p0_2.x,p0_2.y,p0_2.z, n.x,n.y,n.z, 0,0});
        verts.insert(verts.end(), {p1_2.x,p1_2.y,p1_2.z, n.x,n.y,n.z, 1,0});
        verts.insert(verts.end(), {p2_2.x,p2_2.y,p2_2.z, n.x,n.y,n.z, 1,1});
        verts.insert(verts.end(), {p2_2.x,p2_2.y,p2_2.z, n.x,n.y,n.z, 1,1});
        verts.insert(verts.end(), {p3_2.x,p3_2.y,p3_2.z, n.x,n.y,n.z, 0,1});
        verts.insert(verts.end(), {p0_2.x,p0_2.y,p0_2.z, n.x,n.y,n.z, 0,0});
    }

    void placeTrees() {
        trees.clear();

        // v2 specification: 20 perimeter + 6 inner = 26 trees
        // Perimeter (just inside inner fence, 5m inset)
        float perimX = 128.0f, perimZ = 88.0f;
        // North side
        for (float x = -perimX; x <= perimX; x += 26.0f) {
            trees.push_back({glm::vec3(x, 0.0f, -perimZ), 1.0f + (rand() % 3) * 0.1f});
        }
        // South side
        for (float x = -perimX; x <= perimX; x += 26.0f) {
            trees.push_back({glm::vec3(x, 0.0f, perimZ), 1.0f + (rand() % 3) * 0.1f});
        }
        // Inner trees near Appellplatz
        trees.push_back({glm::vec3(10.0f, 0.0f, -15.0f), 1.3f});
        trees.push_back({glm::vec3(60.0f, 0.0f, -10.0f), 1.1f});
        trees.push_back({glm::vec3(15.0f, 0.0f, 30.0f), 1.2f});
        trees.push_back({glm::vec3(55.0f, 0.0f, 25.0f), 1.0f});
        trees.push_back({glm::vec3(-65.0f, 0.0f, 5.0f), 1.15f});
        trees.push_back({glm::vec3(-30.0f, 0.0f, -50.0f), 1.25f});
    }
};

#endif
