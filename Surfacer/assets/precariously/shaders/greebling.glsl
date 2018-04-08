vertex:
#version 150
uniform mat4 ciModelViewProjection;
uniform float ciElapsedSeconds;
uniform float swayPeriod;
uniform float swayFactors[4];


in vec4 ciPosition;
in vec2 ciNormal;
in vec2 ciTexCoord0;
in vec2 ciTexCoord1;
in vec2 ciTexCoord2;
in vec4 ciColor;
in int ciBoneIndex;

out vec2 Up;
out vec2 TexCoord;
out vec2 VertexPosition;
out vec2 Random;
out vec4 Color;


void main(void) {
    Up = ciNormal.xy;
    TexCoord = ciTexCoord0;
    VertexPosition = ciTexCoord1;
    Random = ciTexCoord2;
    Color = ciColor;

    float time = ciElapsedSeconds * (1 + 0.25 * ((Random.y * 2) - 1));
    vec4 right = vec4(Up.y, -Up.x, 0, 0);    
    vec4 wiggle = cos((time + (Random.x * swayPeriod)) / swayPeriod) * right * swayFactors[ciBoneIndex];
    
    gl_Position = ciModelViewProjection * (ciPosition + wiggle * VertexPosition.y);
}

fragment:
#version 150
uniform sampler2D uTex0;

in vec2 Up;
in vec2 TexCoord;
in vec2 VertexPosition;
in vec2 Random;
in vec4 Color;

out vec4 oColor;

void main(void) {
    vec4 tex = texture(uTex0, TexCoord);
    tex.a = floor(tex.a);
    
    // NOTE: additive blending requires premultiplication
    oColor.rgb = Color.rgb * tex.rgb * tex.a;
    oColor.a = Color.a * tex.a;
}