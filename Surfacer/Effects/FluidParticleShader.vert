attribute float ParticleOpacity;

varying vec2 TexCoord;
varying float Opacity;

void main()
{
	TexCoord = gl_MultiTexCoord0.st;
	Opacity = ParticleOpacity;
	
	gl_Position = ftransform();
}