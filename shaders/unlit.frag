#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform vec3 objectColor;
uniform sampler2D texture1;
uniform bool useTexture;

void main()
{
    if (useTexture) {
        vec4 texColor = texture(texture1, TexCoord);
        FragColor = vec4(objectColor * texColor.rgb, texColor.a);
    } else {
        FragColor = vec4(objectColor, 1.0);
    }
}
