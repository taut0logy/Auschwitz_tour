#version 330 core
layout (location = 0) in vec3 aPos;

out vec3 LocalPos;

uniform mat4 view;
uniform mat4 projection;

void main()
{
    LocalPos = aPos;
    // Remove translation from view matrix so skybox follows camera
    mat4 viewNoTranslation = mat4(mat3(view));
    vec4 pos = projection * viewNoTranslation * vec4(aPos, 1.0);
    // Set z = w so it renders at max depth (behind everything)
    gl_Position = pos.xyww;
}
