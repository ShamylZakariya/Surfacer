uniform float PaletteSize, rPaletteSize;
uniform vec4 Color;
varying float EdgeTransit; // goes from -1 to 1

void main()
{
	float alpha = 1.0 - abs( EdgeTransit );
	alpha = (alpha * alpha * alpha) + 0.1;
	
	vec4 color = vec4( Color.rgb, Color.a * alpha );
	vec4 palettized = vec4( ivec4( color * PaletteSize )) * rPaletteSize;


	gl_FragColor = palettized;
}