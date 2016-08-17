varying vec4 VertexColor;

void main()
{
	VertexColor = gl_Color;
	gl_Position = ftransform();
}