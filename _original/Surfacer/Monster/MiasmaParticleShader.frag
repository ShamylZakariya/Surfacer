uniform sampler2D Texture0;
uniform sampler2D Texture1; // noise texture
uniform vec2 NoiseOffset; // noise offset
uniform float NoiseTextureAlpha;

varying vec2 TexCoord;
varying vec4 Color;

const float NoiseTextureSize = 256.0;

void main()
{
	vec2 noiseTextureCoord = mod( gl_FragCoord.xy + NoiseOffset, NoiseTextureSize ) / NoiseTextureSize;
	vec4 texColor = texture2D( Texture0, TexCoord ) * Color;
	vec4 noiseColor = texture2D( Texture1, noiseTextureCoord );
	vec4 fragColor = mix( texColor, texColor * noiseColor, noiseColor.a * NoiseTextureAlpha );
	
	gl_FragColor = vec4( fragColor.rgb, texColor.a );


//	vec2 noiseTextureCoord = mod( gl_FragCoord.xy + NoiseOffset, NoiseTextureSize ) / NoiseTextureSize;
//	vec4 texColor = texture2D( Texture0, TexCoord ) * Color;
//	vec4 noiseColor = texture2D( Texture1, noiseTextureCoord );
//	vec4 noiseOffset = (noiseColor - vec4(0.5)) * 2.0;
//	
//	gl_FragColor = vec4( texColor.rgb + (NoiseTextureAlpha * noiseOffset.rgb), texColor.a );
}