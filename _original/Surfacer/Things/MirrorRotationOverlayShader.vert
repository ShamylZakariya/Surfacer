attribute float DistanceFromOriginAttr;

varying float DistanceFromOrigin;

void main()
{
	DistanceFromOrigin = DistanceFromOriginAttr;
	gl_Position = ftransform();
}