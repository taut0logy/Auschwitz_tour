#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D fontTex;

void main()
{
    vec4 texel = texture(fontTex, TexCoord);
    if (texel.a < 0.1)
        discard;
    FragColor = vec4(1.0, 1.0, 1.0, texel.a);
}
