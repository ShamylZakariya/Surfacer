uniform sampler2DRect InputTex, NoiseTex;
uniform vec2 NoiseTexSize, NoiseTexOffset;
uniform float Strength;

void main()
{
	vec4 color = texture2DRect( InputTex, gl_FragCoord.st );
	vec3 noise = texture2DRect( NoiseTex, mod( gl_FragCoord.st + NoiseTexOffset, NoiseTexSize )).rgb;
	noise = (noise * 2.0) - vec3(1.0,1.0,1.0);

	gl_FragColor = vec4( color.rgb + (Strength * noise), color.a );
}