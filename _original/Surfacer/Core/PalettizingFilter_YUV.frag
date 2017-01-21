//
//	Uniforms:
//		Palette sizes are [0,255] with 255 as identity
//

uniform sampler2DRect InputTex;
uniform float Strength;
uniform vec4 PaletteSize, rPaletteSize;

vec3 r2y( in vec3 RGB )
{
	return vec3(
		 (0.257 * RGB.r) + (0.504 * RGB.g) + (0.098 * RGB.b) + 0.0625,
	    -(0.148 * RGB.r) - (0.291 * RGB.g) + (0.439 * RGB.b) + 0.5,
	     (0.439 * RGB.r) - (0.368 * RGB.g) - (0.071 * RGB.b) + 0.5 );
}

vec3 y2r( in vec3 YUV )
{
	return vec3( 
		(1.164 * (YUV.x - 0.0625)) + (1.596 * (YUV.z - 0.5)),
		(1.164 * (YUV.x - 0.0625)) - (0.813 * (YUV.z - 0.5)) - (0.391 * (YUV.y - 0.5)),
		(1.164 * (YUV.x - 0.0625)) + (2.018 * (YUV.y - 0.5))
		);
}

void main() 
{
	vec4 color = texture2DRect( InputTex, gl_FragCoord.st );
	vec3 yuv = r2y( color.rgb );
	vec3 palettizedYuv = vec3( ivec3( yuv * PaletteSize.xyz )) * rPaletteSize.xyz;
	float palettizedAlpha = float( int( color.a * PaletteSize.w )) * rPaletteSize.w;
	vec4 palettized = vec4( palettizedYuv.r, palettizedYuv.g, palettizedYuv.b, palettizedAlpha );

	gl_FragColor = mix( color, palettized, Strength );
	
}
