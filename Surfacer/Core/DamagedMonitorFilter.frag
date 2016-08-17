uniform sampler2DRect InputTex;
uniform float Strength;
uniform vec3 SampleSizes;
uniform vec4 ColorScale;
uniform vec4 ColorOffset;
uniform vec2 RedOffset;
uniform vec2 GreenOffset;
uniform vec2 BlueOffset;
uniform float ScanlineStrength;


void main()
{
	//
	//	Compute per-channel sample size
	//

	vec3 
		SampleSizes = max( Strength * SampleSizes, vec3(1,1,1) );

	//
	//	Compute downsampled lookup frag coord per channel, and then 
	//	lookup downsampled red/alpha, green/alpha, & blue/alpha
	//

	vec2 
		RedDownsampled		= vec2( floor( gl_FragCoord.st / SampleSizes.r )),
		GreenDownsampled	= vec2( floor( gl_FragCoord.st / SampleSizes.g )),
		BlueDownsampled		= vec2( floor( gl_FragCoord.st / SampleSizes.b )),

		RedFragCoord		= RedDownsampled   * SampleSizes.r,
		GreenFragCoord		= GreenDownsampled * SampleSizes.g,
		BlueFragCoord		= BlueDownsampled  * SampleSizes.b,

		RA = texture2DRect( InputTex, RedFragCoord   + floor(Strength * SampleSizes.r * RedOffset   )).ra,
		GA = texture2DRect( InputTex, GreenFragCoord + floor(Strength * SampleSizes.g * GreenOffset )).ga,
		BA = texture2DRect( InputTex, BlueFragCoord  + floor(Strength * SampleSizes.b * BlueOffset  )).ba;

	//
	//	Mix the separated red/green/blue and we're summing their respective alphas because it looks better than the average
	//

	vec4 result = (ColorScale * vec4( RA.x, GA.x, BA.x, min( (RA.y + GA.y + BA.y), 1.0 ) )) + ColorOffset;

	//
	//	Mix in scanline effect
	//

	float scanlineAlpha = step( mod( gl_FragCoord.t, 2.0 ), 0.5 );
	float columnAlpha = 0.75 + (0.25 * step( mod( gl_FragCoord.s, 4.0 ) * 0.25, 0.75 ));
	float scanlineEffect = scanlineAlpha * columnAlpha;
	
	result.a *= mix( scanlineEffect, 1.0, 1.0-Strength*ScanlineStrength);

	gl_FragColor = result;
}