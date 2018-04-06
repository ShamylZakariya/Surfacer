vertex:
#version 150
uniform mat4 ciModelViewProjection;

in vec4 ciPosition;
in vec2 ciTexCoord0;
in vec4 ciColor;

out vec2 TexCoord;
out vec4 Color;

void main(void) {
    gl_Position = ciModelViewProjection * ciPosition;
    TexCoord = ciTexCoord0;
    Color = ciColor;
}

fragment:
#version 150
uniform sampler2D uTex0;

in vec2 TexCoord;
in vec4 Color;

out vec4 oColor;

void main(void) {
    float alpha = round(texture(uTex0, TexCoord).r);
    
    // NOTE: additive blending requires premultiplication
    oColor.rgb = Color.rgb * alpha;
    oColor.a = Color.a * alpha;
}
