attribute float Edge;

varying float EdgeTransit;

void main()
{
	EdgeTransit = Edge;
	gl_Position = ftransform();
}