uniform sampler2D ModulationTexture;
uniform sampler2D ColorTexture;
uniform vec2 ColorTextureCoord;
uniform vec4 Color;

varying vec2 TextureCoord;
varying vec4 HeatColor;

void main()
{
	vec4 modulation = texture2D( ModulationTexture, TextureCoord );
	vec4 color = (Color*texture2D( ColorTexture, ColorTextureCoord )) + HeatColor;
	color.rgb += ((modulation.rgb * 2.0) - 1.0) * modulation.a;
	
	gl_FragColor = vec4( color.rgb, color.a * HeatColor.a );
}