uniform sampler2D ParticleTex;

varying vec2 TexCoord;
varying float Opacity;

void main()
{
	// we use GL_ONE, GL_ONE blending, so opacity is just a multiply
	gl_FragColor = texture2D( ParticleTex, TexCoord ) * Opacity;
}