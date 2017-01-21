uniform sampler2D	Texture0;
uniform vec2		Texture0Size;
uniform vec2		Texture0Offset;
uniform vec4		Texture0Color;

uniform sampler2D	Texture1;
uniform vec2		Texture1Size;
uniform vec2		Texture1Offset;
uniform vec4		Texture1Color;

uniform sampler2D	Texture2;
uniform vec2		Texture2Size;
uniform vec2		Texture2Offset;
uniform vec4		Texture2Color;

void main()
{
	vec2 t0coord = mod( gl_FragCoord.xy + Texture0Offset, Texture0Size ) / Texture0Size;
	t0coord.t = 1.0 - t0coord.t;
	vec4 t0 = Texture0Color * texture2D( Texture0, t0coord );

	vec2 t1coord = mod( gl_FragCoord.xy + Texture1Offset, Texture1Size ) / Texture1Size;	
	t1coord.t = 1.0 - t1coord.t;
	vec4 t1 = Texture1Color * texture2D( Texture1, t1coord );

	vec2 t2coord = mod( gl_FragCoord.xy + Texture2Offset, Texture2Size ) / Texture2Size;	
	t2coord.t = 1.0 - t2coord.t;
	vec4 t2 = Texture2Color * texture2D( Texture2, t2coord );

	vec4 color = mix( t0, t1, t1.a );
	color = mix( color, t2, t2.a );
	
	gl_FragColor = vec4(color.rgb,1.0);
}