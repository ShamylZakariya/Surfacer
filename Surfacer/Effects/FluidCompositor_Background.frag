uniform sampler2DRect InputImage;
uniform sampler2D BackgroundImage;
uniform vec2 BackgroundImageSize;

uniform float LumaPower;
uniform float Hue;
uniform float Opacity;
uniform vec4 HighlightColor;
uniform float HighlightLookupDistPx;
uniform float Shininess;

#define ALPHA_EPSILON (1.0/255.0)
#define HIGHLIGHT_ENABLED
#define HIGHLIGHT_DEBUG 0

void main()
{
	float srcLuma = texture2DRect( InputImage, gl_FragCoord.st ).r;	
	float luma = pow( srcLuma, LumaPower );

	if ( luma < ALPHA_EPSILON ) discard;
	
	vec4 fluidColor = texture2D( BackgroundImage, mod(gl_FragCoord.st,BackgroundImageSize) / BackgroundImageSize );
	fluidColor.a *= Opacity;
	
	if ( fluidColor.a < ALPHA_EPSILON ) discard;
	
	#ifdef HIGHLIGHT_ENABLED
		if ( HighlightColor.a > ALPHA_EPSILON )
		{
			float 
				lumaSouth = texture2DRect( InputImage, vec2( gl_FragCoord.s, gl_FragCoord.t - HighlightLookupDistPx * 0.5 )).r,
				lumaNorth = texture2DRect( InputImage, vec2( gl_FragCoord.s, gl_FragCoord.t + HighlightLookupDistPx * 0.5 )).r;

			lumaSouth = pow( lumaSouth, LumaPower );
			lumaNorth = pow( lumaNorth, LumaPower );
			
			float 
				dullHighlight = step( lumaNorth, lumaSouth ),
				shinyHighlight = step( 0.125, dullHighlight * srcLuma ),
				highlight = mix( dullHighlight, shinyHighlight, Shininess );
			
			#if HIGHLIGHT_DEBUG
				fluidColor = mix( vec4(0,0,0,1), vec4(1,1,1,1), highlight );
			#else
				fluidColor.rgb += HighlightColor.rgb * highlight * HighlightColor.a;
			#endif
		}
	#endif
	
	gl_FragColor = fluidColor;
}