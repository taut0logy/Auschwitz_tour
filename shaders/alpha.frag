#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    vec3 emissive;
    float shininess;
};

struct DirLight {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

uniform vec3      viewPos;
uniform vec3      globalAmbient;
uniform Material  material;
uniform DirLight  dirLight;

uniform sampler2D texture1;
uniform bool      useTexture;
uniform float     texRepeat;
uniform float     objectAlpha;

void main()
{
    vec3 N = normalize(Normal);
    vec3 V = normalize(viewPos - FragPos);

    vec3 matDiff = material.diffuse;
    float alpha = objectAlpha;

    if (useTexture) {
        vec2 tc = TexCoord * texRepeat;
        vec4 texColor = texture(texture1, tc);
        if (texColor.a < 0.1)
            discard;
        matDiff = texColor.rgb;
        alpha = texColor.a;
    }

    vec3 matAmb = matDiff * 0.3;

    // Simple directional light only for alpha objects
    vec3 L  = normalize(-dirLight.direction);
    float d = max(dot(N, L), 0.0);
    vec3 result = globalAmbient * matAmb
                + dirLight.ambient * matAmb
                + dirLight.diffuse * d * matDiff;

    result += material.emissive;
    FragColor = vec4(result, alpha);
}
