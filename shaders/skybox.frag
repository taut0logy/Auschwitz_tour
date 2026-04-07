#version 330 core
out vec4 FragColor;

in vec3 LocalPos;

uniform vec3 skyTopColor;
uniform vec3 skyHorizonColor;

// Sun glow
uniform vec3  sunGlowDir;      // normalized direction to the sun
uniform float sunGlowStrength; // 0.0 at night, up to 2.0 at dawn/dusk

void main()
{
    vec3 dir = normalize(LocalPos);
    // Gradient from horizon to zenith
    float t = clamp(dir.y, 0.0, 1.0);
    t = t * t;
    vec3 color = mix(skyHorizonColor, skyTopColor, t);

    // Sun glow halo
    if (sunGlowStrength > 0.01) {
        float sunDot = max(dot(dir, sunGlowDir), 0.0);
        float glow = pow(sunDot, 64.0) * sunGlowStrength;
        float halo = pow(sunDot, 8.0) * sunGlowStrength * 0.15;
        color += vec3(1.0, 0.95, 0.7) * glow;
        color += vec3(1.0, 0.85, 0.5) * halo;
    }

    FragColor = vec4(color, 1.0);
}
