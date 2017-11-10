//
//  Terrain.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 3/30/17.
//
//

#include "Terrain.hpp"

using namespace core;

namespace terrain {

#pragma mark - TerrainObject

	TerrainObject::TerrainObject(string name, WorldRef world, int drawLayer):
	Object(name),
	_world(world)
	{
		addComponent(make_shared<TerrainDrawComponent>(drawLayer));
		addComponent(make_shared<TerrainPhysicsComponent>());
	}

	TerrainObject::~TerrainObject()
	{}

	void TerrainObject::onReady(StageRef stage) {
		Object::onReady(stage);
		_world->setObject(shared_from_this());
	}

	void TerrainObject::onCleanup() {
		_world.reset();
	}

	void TerrainObject::step(const time_state &timeState) {
		_world->step(timeState);
	}

	void TerrainObject::update(const time_state &timeState) {
		_world->update(timeState);
	}

#pragma mark - TerrainDrawComponent

	/*
	 WorldRef _world;
	 int _drawLayer;
	 */
	
	void TerrainDrawComponent::onReady(ObjectRef parent, StageRef stage) {
		_world = dynamic_pointer_cast<TerrainObject>(parent)->getWorld();
	}

	void TerrainDrawComponent::draw(const render_state &renderState) {
		_world->draw(renderState);
	}

#pragma mark - TerrainPhysicsComponent

	void TerrainPhysicsComponent::onReady(ObjectRef parent, StageRef stage) {
		_world = dynamic_pointer_cast<TerrainObject>(parent)->getWorld();
	}

	cpBB TerrainPhysicsComponent::getBB() const {
		return cpBBInfinity;
	}

	vector<cpBody*> TerrainPhysicsComponent::getBodies() const {
		vector<cpBody*> bodies;
		for (terrain::GroupBaseRef g : _world->getDynamicGroups()) {
			bodies.push_back(g->getBody());
		}
		bodies.push_back(_world->getStaticGroup()->getBody());
		return bodies;
	}
	
#pragma mark - MouseCutterComponent
	
	MouseCutterComponent::MouseCutterComponent(terrain::TerrainObjectRef terrain, float radius, int dispatchReceiptIndex):
	InputComponent(dispatchReceiptIndex),
	_cutting(false),
	_radius(radius),
	_terrain(terrain)
	{}
	
	bool MouseCutterComponent::mouseDown( const ci::app::MouseEvent &event ) {
		_mouseScreen = event.getPos();
		_mouseWorld = getStage()->getViewport()->screenToWorld(_mouseScreen);
		
		_cutting = true;
		_cutStart = _cutEnd = _mouseWorld;
		
		return true;
	}
	
	bool MouseCutterComponent::mouseUp( const ci::app::MouseEvent &event ){
		if (_cutting) {
			if (_terrain) {
				const float radius = _radius / getStage()->getViewport()->getScale();
				_terrain->getWorld()->cut(_cutStart, _cutEnd, radius);
			}
			
			_cutting = false;
		}
		return false;
	}
	
	bool MouseCutterComponent::mouseMove( const ci::app::MouseEvent &event, const ivec2 &delta ) {
		_mouseScreen = event.getPos();
		_mouseWorld = getStage()->getViewport()->screenToWorld(_mouseScreen);
		return false;
	}
	
	bool MouseCutterComponent::mouseDrag( const ci::app::MouseEvent &event, const ivec2 &delta ) {
		_mouseScreen = event.getPos();
		_mouseWorld = getStage()->getViewport()->screenToWorld(_mouseScreen);
		if (_cutting) {
			_cutEnd = _mouseWorld;
		}
		return false;
	}
	
#pragma mark - MouseCutterDrawComponent
	
	MouseCutterDrawComponent::MouseCutterDrawComponent(ColorA color):
	_color(color)
	{}
	
	void MouseCutterDrawComponent::onReady(ObjectRef parent, StageRef stage){
		DrawComponent::onReady(parent,stage);
		_cutterComponent = getSibling<MouseCutterComponent>();
	}
	
	void MouseCutterDrawComponent::draw(const render_state &renderState){
		if (shared_ptr<MouseCutterComponent> cc = _cutterComponent.lock()) {
			if (cc->isCutting()) {
				float radius = cc->getRadius() / renderState.viewport->getScale();
				vec2 dir = cc->getCutEnd() - cc->getCutStart();
				float len = ::length(dir);
				if (len > 1e-2) {
					dir /= len;
					float angle = atan2(dir.y, dir.x);
					vec2 center = (cc->getCutStart()+cc->getCutEnd()) * 0.5;
					
					mat4 M = translate(vec3(center.x, center.y, 0)) * rotate(angle, vec3(0,0,1));
					gl::ScopedModelMatrix smm;
					gl::multModelMatrix(M);
					
					gl::color(_color);
					gl::drawSolidRect(Rectf(-len/2, -radius, +len/2, +radius));
				}
			}
			
		}
	}


} // namespace terrain
