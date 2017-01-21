varying vec2 IconTexCoord;

void main()
{
	IconTexCoord = gl_MultiTexCoord0.st;
	gl_Position = ftransform();
}