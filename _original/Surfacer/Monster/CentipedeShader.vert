varying vec2 TextureCoord;

void main()
{
	TextureCoord = gl_MultiTexCoord0.xy;
	gl_Position = ftransform();
}