varying vec2 TexCoord;
varying vec4 Color;

void main()
{
	TexCoord = gl_MultiTexCoord0.st;
	Color = gl_Color;

	gl_Position = ftransform();
}