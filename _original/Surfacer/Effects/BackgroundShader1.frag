uniform sampler2D	Texture0;
uniform vec2		Texture0Size;
uniform vec2		Texture0Offset;
uniform vec4		Texture0Color;

void main()
{
	vec2 t0coord = mod( gl_FragCoord.xy + Texture0Offset, Texture0Size ) / Texture0Size;
	t0coord.t = 1.0 - t0coord.t;
	vec4 t0 = Texture0Color * texture2D( Texture0, t0coord );

	gl_FragColor = vec4(t0.rgb,1.0);
}