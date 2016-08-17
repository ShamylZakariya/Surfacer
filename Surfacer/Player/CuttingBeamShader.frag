uniform float PaletteSize, rPaletteSize;
varying vec4 VertexColor;

void main()
{	
	gl_FragColor = vec4( ivec4( VertexColor * PaletteSize )) * rPaletteSize;
}