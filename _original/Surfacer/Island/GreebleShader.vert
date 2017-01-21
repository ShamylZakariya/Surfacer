varying vec2 TexCoord;
varying vec4 Color;

void main()
{
	// we pass terrain tex coords in gl_MultiTexCoord0, and greeble tex coords in gl_MultiTexCoord1
	TexCoord = gl_MultiTexCoord1.st;
	Color = gl_Color;

	gl_Position = ftransform();
}