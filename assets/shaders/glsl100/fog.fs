#version 100

precision mediump float;

varying vec3 fragPosition;
varying vec2 fragTexCoord;
varying vec4 fragColor;
varying vec3 fragNormal;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

uniform vec3 viewPos;
uniform float fogDensity;

void main()
{
  const vec4 fogColor = vec4(0, 0, 0, 1.0);

  vec4 texelColor = texture2D(texture0, fragTexCoord);
  vec4 finalColor = texelColor*colDiffuse;
  float dist = length(viewPos - fragPosition);
  float fogFactor = 1.0/exp((dist*fogDensity)*(dist*fogDensity));

  fogFactor = clamp(fogFactor, 0.0, 1.0);

  gl_FragColor = mix(fogColor, finalColor, fogFactor);
}
