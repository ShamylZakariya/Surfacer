uniform sampler2D Texture0;

varying vec2 TexCoord;
varying vec4 Color;

void main()
{
	// controlled-additive-blending requires premultiplied color from our textures
	vec4 texColor = texture2D( Texture0, TexCoord );
	texColor.rgb *= texColor.w; 

	gl_FragColor = texColor * Color;
}