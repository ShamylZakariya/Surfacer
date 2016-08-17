uniform sampler2D ModulationTexture;
uniform vec4 Color;

varying vec2 TextureCoord;

void main()
{
	vec4 modulation = texture2D( ModulationTexture, TextureCoord );
	vec4 color = Color;
	color.rgb += ((modulation.rgb - 0.5) * 2.0) * modulation.a;

	gl_FragColor = color;
}