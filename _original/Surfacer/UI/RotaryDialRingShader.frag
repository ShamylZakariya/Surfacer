uniform sampler2D	RingTex;
uniform float		Value;
uniform vec4		BaseColor;
uniform vec4		HighlightColor;

varying vec2		TexCoord;


void main()
{
	vec4 ring = texture2D( RingTex, TexCoord );

	float t = step( ring.r, Value );
	vec4 color = mix( BaseColor, HighlightColor, t );
	color.a *= ring.a;

	gl_FragColor = color;
}