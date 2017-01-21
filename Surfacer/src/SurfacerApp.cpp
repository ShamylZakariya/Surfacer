#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class SurfacerApp : public App {
  public:
	void setup() override;
	void mouseDown( MouseEvent event ) override;
	void update() override;
	void draw() override;
};

void SurfacerApp::setup()
{
}

void SurfacerApp::mouseDown( MouseEvent event )
{
}

void SurfacerApp::update()
{
}

void SurfacerApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) ); 
}

CINDER_APP( SurfacerApp, RendererGl )
