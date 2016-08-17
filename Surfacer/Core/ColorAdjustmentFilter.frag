//
//	Uniforms:
//		Saturation [0,N] with 1 as identity
//		Contrast [0,N] with 1 as identity
//		Brightness [-1,1] with 1 as identity
//

uniform sampler2DRect InputTex;
uniform float Strength, Saturation, Contrast, Brightness;


void main()
{
	const vec3 L = vec3( 0.299, 0.587, 0.114 );
	vec4 source = texture2DRect( InputTex, gl_FragCoord.st );
	
	// perform saturation
	float luma = dot( L, source.rgb );
	vec3 color = mix( vec3(luma), source.rgb, Saturation );
	
	// apply brightness offset
	color = clamp( color + vec3( Brightness ), 0.0, 1.0 );

	// contrast -- value is 0 -> N where 1 is no contrast adjustment
	color = clamp( vec3( 0.5 ) + ((color) - vec3(0.5)) * vec3( Contrast ), 0.0, 1.0 );

	// now mix in the result
	gl_FragColor = vec4( mix( source.rgb, color, Strength), source.a );
}