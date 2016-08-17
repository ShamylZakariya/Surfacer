uniform sampler2D IconTex;
uniform vec4 Color;
varying vec2 IconTexCoord;

void main()
{
	// we use the icon soleley for its alpha channel
	float iconMask = texture2D( IconTex, IconTexCoord ).a;
	gl_FragColor = vec4( Color.rgb, Color.a * iconMask );
}