#version 330

in vec2 fragTexCoord;
in vec4 fragColor;
in vec3 fragPosition;
in vec3 fragNormal;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

out vec4 finalColor;

uniform vec3 viewPos;
uniform float fogDensity;

void main() {
  const vec4 fogColor = vec4(0, 0, 0, 1.0);

  vec4 texelColor = texture(texture0, fragTexCoord);
  finalColor = texelColor*colDiffuse;
  float dist = length(viewPos - fragPosition);
  float fogFactor = 1.0/exp((dist*fogDensity)*(dist*fogDensity));

  fogFactor = clamp(fogFactor, 0.0, 1.0);

  finalColor = mix(fogColor, finalColor, fogFactor);
}

