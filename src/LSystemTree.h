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
#include <cstdint>
#include "Shader.h"

// ================================================================
// LSystemTree: Fractal L-system trees (poplar style, 4 iterations)
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
        std::string sentence = "X";
        int iterations = 5;

        // X = bud (draws leaves), F = branch segment
        for (int i = 0; i < iterations; i++) {
            std::string next;
            for (char c : sentence) {
                if (c == 'X') next += "F-[[X]+X]+F[+FX]-X";
                else if (c == 'F') next += "FF";
                else next += c;
            }
            sentence = next;
        }

        // Generate geometry from sentence using 3D turtle graphics
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
    static constexpr float ANGLE = 25.0f;
    static constexpr float INIT_LENGTH = 0.45f;
    static constexpr float INIT_RADIUS = 0.20f;
    
    struct TurtleState {
        glm::vec3 pos;
        glm::vec3 dir;
        glm::vec3 right;
        glm::vec3 up;
        float radius;
    };

    static float hash01(uint32_t n) {
        n ^= 2747636419u;
        n *= 2654435769u;
        n ^= n >> 16;
        n *= 2654435769u;
        n ^= n >> 16;
        return (float)(n & 0x00FFFFFFu) / 16777215.0f;
    }

    void generateGeometry(const std::string& sentence) {
        std::vector<float> branchVerts;
        std::vector<float> leafVerts;

        TurtleState turtle;
        turtle.pos = glm::vec3(0.0f, 0.0f, 0.0f);
        turtle.dir = glm::vec3(0.0f, 1.0f, 0.0f);
        turtle.right = glm::vec3(1.0f, 0.0f, 0.0f);
        turtle.up = glm::vec3(0.0f, 0.0f, 1.0f);
        turtle.radius = INIT_RADIUS;

        std::stack<TurtleState> stateStack;
        int turnCounter = 0;
        int branchCounter = 0;

        for (char c : sentence) {
            if (c == 'F') {
                // Draw a branch segment
                float radiusRatio = glm::clamp(turtle.radius / INIT_RADIUS, 0.12f, 1.0f);
                float segLength = INIT_LENGTH * (0.40f + 0.60f * radiusRatio);
                glm::vec3 endPos = turtle.pos + turtle.dir * segLength;
                addCylinder(branchVerts, turtle.pos, endPos, turtle.radius);
                turtle.pos = endPos;
            }
            else if (c == '+') {
                // Deterministic turn with small jitter for even but natural spread.
                float jitter = (hash01((uint32_t)(turnCounter + 17)) - 0.5f) * 8.0f;
                float primary = glm::radians(ANGLE + jitter);
                float secondary = glm::radians((hash01((uint32_t)(turnCounter + 911)) - 0.5f) * 10.0f);
                turnCounter++;

                glm::mat4 r1 = glm::rotate(glm::mat4(1.0f), primary, turtle.up);
                turtle.dir = glm::normalize(glm::vec3(r1 * glm::vec4(turtle.dir, 0.0f)));
                turtle.right = glm::normalize(glm::vec3(r1 * glm::vec4(turtle.right, 0.0f)));

                glm::mat4 r2 = glm::rotate(glm::mat4(1.0f), secondary, turtle.right);
                turtle.dir = glm::normalize(glm::vec3(r2 * glm::vec4(turtle.dir, 0.0f)));
                turtle.up = glm::normalize(glm::vec3(r2 * glm::vec4(turtle.up, 0.0f)));
                turtle.right = glm::normalize(glm::cross(turtle.up, turtle.dir));
                turtle.up = glm::normalize(glm::cross(turtle.dir, turtle.right));
            }
            else if (c == '-') {
                float jitter = (hash01((uint32_t)(turnCounter + 29)) - 0.5f) * 8.0f;
                float primary = glm::radians(-ANGLE + jitter);
                float secondary = glm::radians((hash01((uint32_t)(turnCounter + 1237)) - 0.5f) * 10.0f);
                turnCounter++;

                glm::mat4 r1 = glm::rotate(glm::mat4(1.0f), primary, turtle.up);
                turtle.dir = glm::normalize(glm::vec3(r1 * glm::vec4(turtle.dir, 0.0f)));
                turtle.right = glm::normalize(glm::vec3(r1 * glm::vec4(turtle.right, 0.0f)));

                glm::mat4 r2 = glm::rotate(glm::mat4(1.0f), secondary, turtle.right);
                turtle.dir = glm::normalize(glm::vec3(r2 * glm::vec4(turtle.dir, 0.0f)));
                turtle.up = glm::normalize(glm::vec3(r2 * glm::vec4(turtle.up, 0.0f)));
                turtle.right = glm::normalize(glm::cross(turtle.up, turtle.dir));
                turtle.up = glm::normalize(glm::cross(turtle.dir, turtle.right));
            }
            else if (c == '[') {
                stateStack.push(turtle);
                // Branches get thinner
                turtle.radius *= 0.65f;
                if (turtle.radius < 0.02f) turtle.radius = 0.02f;

                // Use a golden-angle roll for evenly distributed radial branching.
                float rollDeg = fmodf(137.50776f * (float)(branchCounter + 1), 360.0f);
                float roll = glm::radians(rollDeg);
                glm::mat4 rotRoll = glm::rotate(glm::mat4(1.0f), roll, turtle.dir);
                turtle.right = glm::normalize(glm::vec3(rotRoll * glm::vec4(turtle.right, 0.0f)));
                turtle.up = glm::normalize(glm::vec3(rotRoll * glm::vec4(turtle.up, 0.0f)));

                // Small deterministic upward bias so crowns spread instead of collapsing.
                float liftDeg = 10.0f + 10.0f * hash01((uint32_t)(branchCounter + 431));
                glm::mat4 rotLift = glm::rotate(glm::mat4(1.0f), glm::radians(liftDeg), turtle.right);
                turtle.dir = glm::normalize(glm::vec3(rotLift * glm::vec4(turtle.dir, 0.0f)));
                turtle.up = glm::normalize(glm::vec3(rotLift * glm::vec4(turtle.up, 0.0f)));
                turtle.right = glm::normalize(glm::cross(turtle.up, turtle.dir));
                turtle.up = glm::normalize(glm::cross(turtle.dir, turtle.right));
                branchCounter++;
            }
            else if (c == ']') {
                if (!stateStack.empty()) {
                    turtle = stateStack.top();
                    stateStack.pop();
                }
            }
            else if (c == 'X') {
                // X represents the branch tip/bud. Draw nicely formatted leaves here!
                addLeaves(leafVerts, turtle.pos, turtle.dir, turtle.right);
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
        int sides = 5; // Good balance for high density tree rendering
        glm::vec3 dir = glm::normalize(tip - base);
        
        glm::vec3 up(0, 1, 0);
        if (fabs(dir.y) > 0.99f) up = glm::vec3(1, 0, 0);
        glm::vec3 right = glm::normalize(glm::cross(up, dir));
        glm::vec3 forward = glm::cross(dir, right);

        for (int i = 0; i < sides; i++) {
            float a0 = (float)i / sides * 2.0f * glm::pi<float>();
            float a1 = (float)(i + 1) / sides * 2.0f * glm::pi<float>();
            
            glm::vec3 n0 = right * cosf(a0) + forward * sinf(a0);
            glm::vec3 n1 = right * cosf(a1) + forward * sinf(a1);
            
            glm::vec3 b0 = base + n0 * radius;
            glm::vec3 b1 = base + n1 * radius;
            
            // Slight natural taper along the continuous branch segment
            float tipRadius = radius * 0.8f;
            glm::vec3 t0 = tip + n0 * tipRadius;
            glm::vec3 t1 = tip + n1 * tipRadius;

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

    void addLeaves(std::vector<float>& verts,
                   const glm::vec3& pos, const glm::vec3& dir, const glm::vec3& rightAxis) const
    {
        // Build an even radial leaf cluster at each bud.
        // Texture is a single upright leaf with stem at V=0, so each quad grows from base -> tip.
        const int leavesPerBud = 6;
        const float leafW = 0.24f;
        const float leafL = 0.58f;

        glm::vec3 stem = glm::normalize(dir);
        glm::vec3 refUp = (fabsf(stem.y) < 0.95f) ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);
        glm::vec3 tangent = glm::normalize(glm::cross(refUp, stem));
        glm::vec3 bitangent = glm::normalize(glm::cross(stem, tangent));

        if (glm::length(tangent) < 0.001f) tangent = glm::normalize(rightAxis);
        if (glm::length(bitangent) < 0.001f) bitangent = glm::normalize(glm::cross(stem, tangent));

        for (int i = 0; i < leavesPerBud; ++i) {
            float a = glm::two_pi<float>() * ((float)i / (float)leavesPerBud);
            glm::vec3 radial = glm::normalize(cosf(a) * tangent + sinf(a) * bitangent);

            // Tilt leaves outward while still following branch direction.
            glm::vec3 leafDir = glm::normalize(stem * 0.78f + radial * 0.62f);
            glm::vec3 leafRight = glm::normalize(glm::cross(leafDir, radial));
            if (glm::length(leafRight) < 0.001f) leafRight = tangent;

            glm::vec3 base = pos + radial * 0.045f + stem * 0.02f;
            glm::vec3 p0 = base - leafRight * (leafW * 0.5f);
            glm::vec3 p1 = base + leafRight * (leafW * 0.5f);
            glm::vec3 p2 = p1 + leafDir * leafL;
            glm::vec3 p3 = p0 + leafDir * leafL;

            glm::vec3 n = glm::normalize(glm::cross(leafRight, leafDir));

            // V=0 = leaf stem, V=1 = leaf tip.
            verts.insert(verts.end(), {p0.x,p0.y,p0.z, n.x,n.y,n.z, 0,0});
            verts.insert(verts.end(), {p1.x,p1.y,p1.z, n.x,n.y,n.z, 1,0});
            verts.insert(verts.end(), {p2.x,p2.y,p2.z, n.x,n.y,n.z, 1,1});

            verts.insert(verts.end(), {p2.x,p2.y,p2.z, n.x,n.y,n.z, 1,1});
            verts.insert(verts.end(), {p3.x,p3.y,p3.z, n.x,n.y,n.z, 0,1});
            verts.insert(verts.end(), {p0.x,p0.y,p0.z, n.x,n.y,n.z, 0,0});
        }
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
