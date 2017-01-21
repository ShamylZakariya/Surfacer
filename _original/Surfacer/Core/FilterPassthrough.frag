uniform sampler2DRect InputTex;
uniform vec4 Color;

void main()
{
	gl_FragColor = Color * texture2DRect( InputTex, gl_FragCoord.st );
}