varying vec2 TextureCoord;
varying vec4 HeatColor;

attribute float Heat;
attribute float Opacity;

void main()
{
	TextureCoord = gl_MultiTexCoord0.xy;
	HeatColor = vec4( Heat, pow( Heat, 2.0 ), pow( Heat, 5.0 ) * 0.5, Opacity );
	gl_Position = ftransform();
}