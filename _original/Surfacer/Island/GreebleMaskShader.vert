uniform float MaterialScale;

varying vec2 MaterialTexCoord;
varying vec2 ModulationTexCoord;
varying vec2 GreebleMaskTexCoord;
varying vec4 Color;


void main()
{
	MaterialTexCoord = gl_MultiTexCoord0.xy * MaterialScale;
	ModulationTexCoord = gl_MultiTexCoord0.xy;
	GreebleMaskTexCoord = gl_MultiTexCoord1.xy;

	Color = gl_Color;

	gl_Position = ftransform();
}