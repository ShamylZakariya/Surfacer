//
//	Uniforms:
//		Palette sizes are [0,255] with 255 as identity
//

uniform sampler2DRect InputTex;
uniform float Strength;
uniform vec4 PaletteSize, rPaletteSize;

void main() 
{
	vec4 color = texture2DRect( InputTex, gl_FragCoord.st );
	vec4 palettized = vec4( ivec4( color * PaletteSize )) * rPaletteSize;
	
	gl_FragColor = mix( color, palettized, Strength );
}
