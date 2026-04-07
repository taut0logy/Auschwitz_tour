#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
in float Alpha;

uniform sampler2D particleTex;
uniform vec3 particleColor;
uniform bool useParticleTex;

void main()
{
    vec4 color;
    if (useParticleTex) {
        color = texture(particleTex, TexCoord);
        if (color.a < 0.05)
            discard;
        color.rgb *= particleColor;
        color.a *= Alpha;
    } else {
        color = vec4(particleColor, Alpha);
    }
    FragColor = color;
}
