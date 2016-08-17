varying vec2 MaterialTexCoord;
varying vec2 ModulationTexCoord;
varying vec4 Color;


uniform sampler2D MaterialTexture;
uniform sampler2D ModulationTexture;

const float UncuttableThreshold = 3.0/255.0;
const vec4 UncuttableColor = vec4(0,0,0,1);
const float HardnessLumaMin = 0.65;
const float HardnessLumaRange = 0.35;

void main()
{
	vec4 materialColor = texture2D( MaterialTexture, MaterialTexCoord );
	vec4 modulationColor = texture2D( ModulationTexture, ModulationTexCoord );

	float luma = modulationColor.r;
	modulationColor = HardnessLumaMin + (modulationColor * HardnessLumaRange);
	
	if ( luma < UncuttableThreshold ) 
	{
		modulationColor = UncuttableColor;
	}

	vec4 finalColor = materialColor * modulationColor;
	gl_FragColor = vec4( Color.rgb * finalColor.rgb, Color.a );

}