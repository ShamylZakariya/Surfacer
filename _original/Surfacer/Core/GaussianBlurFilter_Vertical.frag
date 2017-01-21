uniform sampler2DRect Image;
uniform vec2 Kernel[__SIZE__];

void main( void )
{
	int i;
	vec4 sample = vec4(0);
	
	for ( i = 0; i < __SIZE__; i++ )
	{
		sample += texture2DRect( Image, gl_FragCoord.st + vec2( 0, Kernel[i].s ) ) * Kernel[i].t;
	}

	gl_FragColor = sample;
}