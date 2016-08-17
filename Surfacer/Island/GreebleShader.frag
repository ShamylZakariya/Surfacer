uniform sampler2D Texture0;

varying vec2 TexCoord;
varying vec4 Color;

void main()
{
	gl_FragColor = texture2D( Texture0, TexCoord ) * Color;
}