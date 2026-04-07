#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in float aAlpha;

out vec2 TexCoord;
out float Alpha;

uniform mat4 view;
uniform mat4 projection;

void main()
{
    TexCoord = aTexCoord;
    Alpha = aAlpha;
    gl_Position = projection * view * vec4(aPos, 1.0);
}
