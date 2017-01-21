uniform sampler2DRect	InputTex;
uniform float			Strength;
uniform vec4			GradientStart;
uniform vec4			GradientEnd;

void main()
{
	// get input color and compute luminance
	vec4 color = texture2DRect( InputTex, gl_FragCoord.st );

	const vec3 L = vec3( 0.299, 0.587, 0.114 );
	float luma = dot( L, color.rgb );

	// compute mapped color
	vec4 mapped = mix( GradientStart, GradientEnd, luma );
	mapped = mix( color, mapped, Strength );

	gl_FragColor = vec4( mapped.rgb, mapped.a * color.a );
}