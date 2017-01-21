uniform float MaterialScale;

varying vec2 MaterialTexCoord;
varying vec2 ModulationTexCoord;
varying vec4 Color;


void main()
{
	MaterialTexCoord = gl_MultiTexCoord0.xy * MaterialScale;
	ModulationTexCoord = gl_MultiTexCoord0.xy;
	Color = gl_Color;

	gl_Position = ftransform();
}