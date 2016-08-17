varying vec2 TexCoord;

void main()
{
	TexCoord = gl_MultiTexCoord0.st;
	gl_Position = ftransform();
}