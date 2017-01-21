uniform vec4 Color;
uniform float DashLength;

varying float DistanceFromOrigin;

void main()
{
	float position = floor( DistanceFromOrigin / DashLength );
	float alpha = 0.125 + 0.875 * mod( position, 2.0 );
	gl_FragColor = vec4(Color.r,Color.g,Color.b,Color.a * alpha);
}