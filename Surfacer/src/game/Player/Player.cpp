//
//  Player.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 4/23/17.
//
//


#include "Player.hpp"

#include <chipmunk/chipmunk_unsafe.h>

#include "GameLevel.hpp"
#include "Xml.hpp"

using namespace core;

namespace player {

#pragma mark - PlayerPhysicsComponent

	namespace {

		void groundContactQuery(cpShape *shape, cpContactPointSet *points, void *data) {
			if ( cpShapeGetCollisionType(shape) != CollisionType::PLAYER && !cpShapeGetSensor(shape)) {
				bool *didContact = (bool*) data;
				*didContact = true;
			}
		}

		struct ground_slope_handler_data {
			double dist;
			cpVect contact, normal, start, end;

			ground_slope_handler_data():
			dist( FLT_MAX ),
			normal(cpv(0,1))
			{}
		};

		void groundSlopeRaycastHandler(cpShape *shape, cpVect point, cpVect normal, cpFloat alpha, void *data) {
			if (cpShapeGetCollisionType( shape ) != CollisionType::PLAYER) {
				ground_slope_handler_data *handlerData = (ground_slope_handler_data*)data;
				double d2 = cpvdistsq(point, handlerData->start);
				if (d2 < handlerData->dist) {
					handlerData->dist = sqrt(d2);
					handlerData->contact = point;
					handlerData->normal = normal;
				}
			}
		}
	}


	/**
		config _config;
		vector<cpBody*> _bodies;
		vector<cpShape*> _shapes;
		vector<cpConstraint*> _constraints;
		bool _flying;
		double _speed;
	 */
	PlayerPhysicsComponent::PlayerPhysicsComponent(config c):
	_config(c),
	_flying(false),
	_speed(0)
	{}

	PlayerPhysicsComponent::~PlayerPhysicsComponent() {
		cpCleanupAndFree(_constraints);
		cpCleanupAndFree(_shapes);
		cpCleanupAndFree(_bodies);
	}

	void PlayerPhysicsComponent::onReady(core::GameObjectRef parent, core::LevelRef level) {
		PhysicsComponent::onReady(parent, level);
	}

	dvec2 PlayerPhysicsComponent::_getGroundNormal() const
	{
		const double
			HalfWidth = getConfig().width * 0.5,
			SegmentRadius = HalfWidth * 0.125,
			CastDistance = 10000;

		const dvec2
			Position(v2(cpBodyGetPosition(getFootBody()))),
			Down = normalize(getSpace()->getGravity(Position)),
			Right = rotateCCW(Down);

		const cpVect
			DownCast = cpvmult(cpv(Down), CastDistance);

		ground_slope_handler_data handlerData;
		cpSpace *space = getSpace()->getSpace();

		handlerData.start = cpv(Position + Right);
		handlerData.end = cpvadd( handlerData.start, DownCast );
		cpSpaceSegmentQuery(space, handlerData.start, handlerData.end, SegmentRadius, CP_SHAPE_FILTER_ALL, groundSlopeRaycastHandler, &handlerData);
		cpVect normal = handlerData.normal;

		handlerData.start = cpv(Position);
		handlerData.end = cpvadd( handlerData.start, DownCast );
		cpSpaceSegmentQuery(space, handlerData.start, handlerData.end, SegmentRadius, CP_SHAPE_FILTER_ALL, groundSlopeRaycastHandler, &handlerData);
		normal = cpvadd( normal, handlerData.normal );

		handlerData.start = cpv(Position - Right);
		handlerData.end = cpvadd( handlerData.start, DownCast );
		cpSpaceSegmentQuery(space, handlerData.start, handlerData.end, SegmentRadius, CP_SHAPE_FILTER_ALL, groundSlopeRaycastHandler, &handlerData);
		normal = cpvadd( normal, handlerData.normal );

		return normalize(v2(normal));
	}

	bool PlayerPhysicsComponent::_isTouchingGround( cpShape *shape ) const
	{
		bool touchingGroundQuery = false;
		cpSpaceShapeQuery( getSpace()->getSpace(), shape, groundContactQuery, &touchingGroundQuery );
		
		return touchingGroundQuery;
	}

#pragma mark - JetpackUnicyclePlayerPhysicsComponent 

	/*
		cpBody *_body, *_wheelBody;
		cpShape *_bodyShape, *_wheelShape, *_groundContactSensorShape;
		cpConstraint *_wheelMotor, *_orientationGear;
		double _wheelRadius, _wheelFriction, _touchingGroundAcc;
		double _jetpackFuelLevel, _jetpackFuelMax;
		dvec2 _up, _characterUp, _groundNormal;
	 */
	JetpackUnicyclePlayerPhysicsComponent::JetpackUnicyclePlayerPhysicsComponent(config c):
	PlayerPhysicsComponent(c),
	_body(nullptr),
	_wheelBody(nullptr),
	_bodyShape(nullptr),
	_wheelShape(nullptr),
	_groundContactSensorShape(nullptr),
	_wheelMotor(nullptr),
	_orientationGear(nullptr),
	_wheelRadius(0),
	_wheelFriction(0),
	_touchingGroundAcc(0),
	_jetpackFuelLevel(0),
	_jetpackFuelMax(0),
	_up(0,0),
	_characterUp(0,0),
	_groundNormal(0,0)
	{}

	JetpackUnicyclePlayerPhysicsComponent::~JetpackUnicyclePlayerPhysicsComponent()
	{}

	// PhysicsComponent
	void JetpackUnicyclePlayerPhysicsComponent::onReady(core::GameObjectRef parent, core::LevelRef level) {
		PlayerPhysicsComponent::onReady(parent, level);

		const PlayerRef player = dynamic_pointer_cast<Player>(parent);
		const PlayerPhysicsComponent::config &config = getConfig();

		_jetpackFuelLevel = _jetpackFuelMax = config.jetpackFuelMax;

		const double
			Width = getConfig().width,
			Height = getConfig().height,
			Density = getConfig().density,
			HalfWidth = Width / 2,
			HalfHeight = Height/ 2,
			Mass = Width * Height * Density,
			Moment = cpMomentForBox( Mass, Width, Height ),
			WheelRadius = HalfWidth * 1.25,
			WheelSensorRadius = WheelRadius * 1.1,
			WheelMass = Density * M_PI * WheelRadius * WheelRadius;

		const cpVect
			WheelPositionOffset = cpv(0,-HalfHeight);

		_totalMass = Mass + WheelMass;
		_body = _add(cpBodyNew( Mass, Moment ));
		cpBodySetPosition(_body, cpv(getConfig().position));

		//
		// lozenge shape for body and motor to orient it
		//

		_bodyShape = _add(cpSegmentShapeNew( _body, cpv(0,-(HalfHeight)), cpv(0,HalfHeight), Width/2 ));
		cpShapeSetFriction( _bodyShape, 0.1 );
		_orientationGear = _add(cpGearJointNew( cpSpaceGetStaticBody(getSpace()->getSpace()), _body, 0, 1));

		//
		// make wheel at bottom of body
		//

		_wheelRadius = WheelRadius;
		_wheelBody = _add(cpBodyNew( WheelMass, cpMomentForCircle( WheelMass, 0, _wheelRadius, cpvzero )));
		cpBodySetPosition( _wheelBody, cpvadd( cpBodyGetPosition( _body ), WheelPositionOffset ));
		_wheelShape = _add(cpCircleShapeNew( _wheelBody, _wheelRadius, cpvzero ));
		_wheelMotor = _add(cpSimpleMotorNew( _body, _wheelBody, 0 ));

		//
		//	pin joint connecting wheel and body
		//

		_add(cpPivotJointNew(_body, _wheelBody, cpBodyLocalToWorld(_wheelBody, cpvzero)));


		//
		//	create sensor for recording ground contact
		//

		_groundContactSensorShape = _add(cpCircleShapeNew( _wheelBody, WheelSensorRadius, cpvzero ));
		cpShapeSetSensor( _groundContactSensorShape, true );

		//
		//	Finalize
		//

		// group just has to be a unique integer so the player parts don't collide with eachother
		cpShapeFilter filter = CollisionFilters::PLAYER;
		filter.group = reinterpret_cast<cpGroup>(player.get());

		for( cpShape *s : getShapes() )
		{
			cpShapeSetUserData( s, player.get() );
			cpShapeSetFilter(s, filter);
			cpShapeSetCollisionType( s, CollisionType::PLAYER );
			cpShapeSetElasticity( s, 0 );
			getSpace()->addShape(s);
		}

		for( cpBody *b : getBodies() )
		{
			cpBodySetUserData( b, player.get() );
			getSpace()->addBody(b);
		}

		for( cpConstraint *c : getConstraints() )
		{
			cpConstraintSetUserData( c, player.get() );
			getSpace()->addConstraint(c);
		}
	}

	void JetpackUnicyclePlayerPhysicsComponent::step(const core::time_state &timeState) {
		PlayerRef player = getGameObjectAs<Player>();

		const PlayerPhysicsComponent::config config = getConfig();

		const bool
			Alive = true, //player->iaAlive(),
			Restrained = false; // player->isRestrained();

		const double
			Vel = getSpeed(),
			Dir = sign( getSpeed() );

		const dvec2
			Gravity = getSpace()->getGravity(v2(cpBodyGetPosition(_wheelBody))),
			Down = normalize(Gravity),
			Right = rotateCCW(Down);

		//
		//	Update touchingGround average (to smooth out vibrations) and ground slope
		//

		{
			_touchingGroundAcc = lrp<double>(0.2,_touchingGroundAcc,_isTouchingGround(_groundContactSensorShape) ? 1.0 : 0.0);

			_groundNormal = normalize( lrp( 0.2, _groundNormal, _getGroundNormal()));

			const double
				SlopeWeight = 0.125,
				DirWeight = 0.5;

			//
			// compute character model up vector based on gravitational direction and the slope,
			// and direction of motion (we lean into motion)
			//

			const dvec2 ActualUp = -Down;
			const dvec2 NewUp = ActualUp + (SlopeWeight * _groundNormal) + dvec2( DirWeight * Dir,0);
			_characterUp = normalize( lrp( 0.2, _characterUp, NewUp ));

			double characterRotation = std::atan2( _characterUp.y, _characterUp.x ) - M_PI_2;
			while (characterRotation < 0) {
				characterRotation += 2 * M_PI;
			}
			while (characterRotation > 2 * M_PI) {
				characterRotation -= 2 * M_PI;
			}

			cpGearJointSetPhase(_orientationGear, characterRotation);

			//
			//	Now update actual up-vector
			//

			_up = normalize(lrp( 0.2, _up, ActualUp));
		}

		//
		// rotate the wheel motor
		//

		if ( Alive && !Restrained )
		{
			const double
				WheelCircumference = 2 * _wheelRadius * M_PI,
				RequiredTurns = Vel / WheelCircumference,
				DesiredMotorRate = 2 * M_PI * RequiredTurns,
				CurrentMotorRate = cpSimpleMotorGetRate( _wheelMotor ),
				TransitionRate = 0.1,
				MotorRate = lrp<double>( TransitionRate, CurrentMotorRate, DesiredMotorRate );

			cpSimpleMotorSetRate( _wheelMotor, MotorRate );
		}
		else
		{
			cpSimpleMotorSetRate( _wheelMotor, 0 );
		}

		//
		//	Apply jetpack impulse
		//

		bool flying = isFlying() && _jetpackFuelLevel > 0;

		if (flying) {
			dvec2 force = -config.jetpackAntigravity * _totalMass * Gravity;
			cpBodyApplyForceAtWorldPoint(_body, cpv(force), cpBodyLocalToWorld(_wheelBody, cpvzero));
			_jetpackFuelLevel -= config.jetpackFuelConsumptionPerSecond * timeState.deltaT;
		} else if (!isFlying()) {
			if (_jetpackFuelLevel < _config.jetpackFuelMax) {
				_jetpackFuelLevel += config.jetpackFuelRegenerationPerSecond * timeState.deltaT;
				_jetpackFuelLevel = min(_jetpackFuelLevel, _config.jetpackFuelMax);
			}
		}

		//
		//	In video games characters can 'steer' in mid-air. It's expected, even if unrealistic.
		//	So we do it here, too.
		//

		if ( !Restrained && !isTouchingGround() )
		{
			const double
				ActualVel = cpBodyGetVelocity( _body ).x;

			double
				baseImpulseToApply = 0,
				baseImpulse = flying ? 1 : 0.5;

			//
			//	We want to apply force so long as applying force won't take us faster
			//	than our maximum walking speed
			//

			if ( int(sign(ActualVel)) == int(sign(Vel)))
			{
				if ( std::abs( ActualVel ) < std::abs( Vel ))
				{
					baseImpulseToApply = baseImpulse;
				}
			}
			else
			{
				baseImpulseToApply = baseImpulse;
			}

			if ( abs( baseImpulseToApply) > 1e-3 )
			{
				dvec2 impulse = Dir * baseImpulseToApply * _totalMass * Right;
				cpBodyApplyImpulseAtWorldPoint( _body, cpv(impulse), cpBodyGetPosition(_body));
			}
		}


		//
		//	Update wheel friction - when crouching, increase friction. When not walking & crouching, increase further.
		//	Note, when kicking off ground we reduce friction. This is to prevent the super-hill-jump effect where running
		//	up a steep slope and jumping causes superman-like flight. Because obviously we're trying to be realistic.
		//

		if ( !Restrained )
		{
			const double
				Friction = getConfig().friction,
				LockdownFriction = 1000 * Friction,
				Stillness = (1-abs( sign( Vel ))),
				WheelFriction = lrp<double>( Stillness, Friction, LockdownFriction );

				_wheelFriction = lrp<double>( 0.35, _wheelFriction, WheelFriction );

			cpShapeSetFriction( _wheelShape, _wheelFriction );
		}
		else
		{
			cpShapeSetFriction( _wheelShape, 0 );
		}

		//
		//	If player has died, release rotation constraint
		//

		if ( !Alive )
		{
			cpConstraintSetMaxForce( _orientationGear, 0 );
			cpConstraintSetMaxBias( _orientationGear, 0 );
			cpConstraintSetErrorBias( _orientationGear, 0 );
			for (cpShape *shape : getShapes()) {
				cpShapeSetFriction(shape, 0);
			}
		}
		else
		{
			// orientation constraint may have been lessened when player was crushed, we always want to ramp it back up
			const double MaxForce = cpConstraintGetMaxForce(_orientationGear);
			if ( MaxForce < 10000 )
			{
				cpConstraintSetMaxForce( _orientationGear, max(MaxForce,10.0) * 1.1 );
			}
			else
			{
				cpConstraintSetMaxForce( _orientationGear, INFINITY );
			}
		}

		//
		//	Draw component needs update its BB for draw dispatch
		//

		player->getDrawComponent()->notifyMoved();
	}

	cpBB JetpackUnicyclePlayerPhysicsComponent::getBB() const {
		return cpBBNewForCircle(cpv(getPosition()), getConfig().height);
	}

	dvec2 JetpackUnicyclePlayerPhysicsComponent::getPosition() const {
		return v2(cpBodyGetPosition( _body ));
	}

	dvec2 JetpackUnicyclePlayerPhysicsComponent::getUp() const {
		return _up;
	}

	dvec2 JetpackUnicyclePlayerPhysicsComponent::getGroundNormal() const {
		return _groundNormal;
	}

	bool JetpackUnicyclePlayerPhysicsComponent::isTouchingGround() const {
		return _touchingGroundAcc >= 0.5;
	}

	cpBody* JetpackUnicyclePlayerPhysicsComponent::getBody() const {
		return _body;
	}

	cpBody* JetpackUnicyclePlayerPhysicsComponent::getFootBody() const {
		return _wheelBody;
	}

	double JetpackUnicyclePlayerPhysicsComponent::getJetpackFuelLevel() const {
		return _jetpackFuelLevel;
	}

	double JetpackUnicyclePlayerPhysicsComponent::getJetpackFuelMax() const {
		return _jetpackFuelMax;
	}

	JetpackUnicyclePlayerPhysicsComponent::capsule JetpackUnicyclePlayerPhysicsComponent::getBodyCapsule() const {
		capsule cap;
		cap.a = v2(cpBodyLocalToWorld(cpShapeGetBody(_bodyShape), cpSegmentShapeGetA(_bodyShape)));
		cap.b = v2(cpBodyLocalToWorld(cpShapeGetBody(_bodyShape), cpSegmentShapeGetB(_bodyShape)));
		cap.radius = cpSegmentShapeGetRadius(_bodyShape);
		return cap;
	}

	JetpackUnicyclePlayerPhysicsComponent::wheel JetpackUnicyclePlayerPhysicsComponent::getFootWheel() const {
		wheel w;
		w.position = v2(cpBodyGetPosition(_wheelBody));
		w.radius = cpCircleShapeGetRadius(_wheelShape);
		w.radians = cpBodyGetAngle(_wheelBody);

		return w;
	}



#pragma mark - PlayerInputComponent

	PlayerInputComponent::PlayerInputComponent() {
		_keyRun = app::KeyEvent::KEY_LSHIFT;
		_keyLeft = app::KeyEvent::KEY_a;
		_keyRight = app::KeyEvent::KEY_d;
		_keyJump = app::KeyEvent::KEY_w;
		_keyCrouch = app::KeyEvent::KEY_s;

		monitorKey( _keyLeft );
		monitorKey( _keyRight );
		monitorKey( _keyJump );
		monitorKey( _keyCrouch );
		monitorKey( _keyRun );
	}

	PlayerInputComponent::~PlayerInputComponent()
	{}

	bool PlayerInputComponent::isRunning() const {
		return isMonitoredKeyDown(_keyRun);
	}

	bool PlayerInputComponent::isGoingRight() const {
		return isMonitoredKeyDown(_keyRight);
	}

	bool PlayerInputComponent::isGoingLeft() const {
		return isMonitoredKeyDown(_keyLeft);
	}

	bool PlayerInputComponent::isJumping() const {
		return isMonitoredKeyDown(_keyJump);
	}

	bool PlayerInputComponent::isCrouching() const {
		return isMonitoredKeyDown(_keyCrouch);
	}

#pragma mark - PlayerDrawComponent

	PlayerDrawComponent::PlayerDrawComponent()
	{}

	PlayerDrawComponent::~PlayerDrawComponent()
	{}

	void PlayerDrawComponent::onReady(GameObjectRef parent, LevelRef level) {
		DrawComponent::onReady(parent, level);
		_physics = getSibling<JetpackUnicyclePlayerPhysicsComponent>();
	}

	cpBB PlayerDrawComponent::getBB() const {
		if (PlayerPhysicsComponentRef ppc = _physics.lock()) {
			return ppc->getBB();
		}

		return cpBBZero;
	}

	void PlayerDrawComponent::draw(const core::render_state &renderState) {

		JetpackUnicyclePlayerPhysicsComponentRef physics = _physics.lock();
		const JetpackUnicyclePlayerPhysicsComponent::wheel FootWheel = physics->getFootWheel();
		const JetpackUnicyclePlayerPhysicsComponent::capsule BodyCapsule = physics->getBodyCapsule();

		// draw the wheel
		gl::color(1,1,1); 
		gl::drawSolidCircle(FootWheel.position, FootWheel.radius, 32);
		gl::color(0,0,0);
		gl::drawLine(FootWheel.position, FootWheel.position + FootWheel.radius * dvec2(cos(FootWheel.radians), sin(FootWheel.radians)));

		// draw the capsule
		{
			double radius = BodyCapsule.radius;
			dvec2 center = (BodyCapsule.a + BodyCapsule.b) * 0.5;
			double len = distance(BodyCapsule.a, BodyCapsule.b);
			dvec2 dir = (BodyCapsule.b - BodyCapsule.a) / len;
			double angle = atan2(dir.y, dir.x);

			gl::ScopedModelMatrix smm;
			mat4 M = glm::translate(dvec3(center.x, center.y, 0)) * glm::rotate(angle, dvec3(0,0,1));
			gl::multModelMatrix(M);

			gl::color(Color(1,1,1));
			gl::drawSolidRoundedRect(Rectf(-len/2, -radius, +len/2, +radius), radius, 8);
		}

		gl::color(1,0,0);
		gl::drawLine(physics->getPosition(), physics->getPosition() + physics->getGroundNormal() * 10.0);
		gl::drawSolidCircle(physics->getPosition(), 1, 12);

		cpBB bb = getBB();
		gl::color(0,1,1);
		gl::drawStrokedRect(Rectf(bb.l, bb.b, bb.r, bb.t));
	}

	VisibilityDetermination::style PlayerDrawComponent::getVisibilityDetermination() const {
		return VisibilityDetermination::FRUSTUM_CULLING;
	}

	int PlayerDrawComponent::getLayer() const {
		return DrawLevels::PLAYER;
	}

	int PlayerDrawComponent::getDrawPasses() const {
		return 1;
	}

#pragma mark - Player

	/**
		config _config;
		PlayerPhysicsComponentRef _physics;
		PlayerDrawComponentRef _drawing;
		PlayerInputComponentRef _input;
	 */
	PlayerRef Player::create(string name, ci::DataSourceRef playerXmlFile, dvec2 position) {
		Player::config config;

		XmlTree playerNode = XmlTree(playerXmlFile).getChild("player");
		config.walkSpeed = util::xml::readNumericAttribute(playerNode, "walkSpeed", 1);
		config.runMultiplier = util::xml::readNumericAttribute(playerNode, "runMultiplier", 10);

		XmlTree physicsNode = playerNode.getChild("physics");

		config.physics.position = position;
		config.physics.width = util::xml::readNumericAttribute(physicsNode, "width", 5);
		config.physics.height = util::xml::readNumericAttribute(physicsNode, "height", 20);
		config.physics.density = util::xml::readNumericAttribute(physicsNode, "density", 1);
		config.physics.friction = util::xml::readNumericAttribute(physicsNode, "friction", 3);

		XmlTree jetpackNode = physicsNode.getChild("jetpack");
		config.physics.jetpackAntigravity = util::xml::readNumericAttribute(jetpackNode, "antigravity", 10);
		config.physics.jetpackFuelMax = util::xml::readNumericAttribute(jetpackNode, "fuelMax", 1);
		config.physics.jetpackFuelConsumptionPerSecond = util::xml::readNumericAttribute(jetpackNode, "consumption", 0.3);
		config.physics.jetpackFuelRegenerationPerSecond = util::xml::readNumericAttribute(jetpackNode, "regeneration", 0.3);

		PlayerRef player = make_shared<Player>(name);
		player->build(config);

		return player;
	}

	Player::Player(string name):
	GameObject(name)
	{}

	Player::~Player(){}

	void Player::update(const time_state &time) {

		//
		//	Synchronize physics rep
		//

		if ( true /*alive()*/ )
		{
			double direction = 0;

			if (_input->isGoingRight()) {
				direction += 1;
			}

			if (_input->isGoingLeft()) {
				direction -= 1;
			}

			_physics->setSpeed( direction * _config.walkSpeed * (_input->isRunning() ? _config.runMultiplier : 1.0) );
			_physics->setFlying( _input->isJumping() );
		}

	}

	TargetTrackingViewportControlComponent::tracking Player::getViewportTracking() const {
		return TargetTrackingViewportControlComponent::tracking(_physics->getPosition(), _physics->getUp(), getConfig().physics.height * 2);
	}

	void Player::build(config c) {
		_config = c;
		_input = make_shared<PlayerInputComponent>();
		_drawing = make_shared<PlayerDrawComponent>();
		_physics = make_shared<JetpackUnicyclePlayerPhysicsComponent>(c.physics);

		addComponent(_input);
		addComponent(_drawing);
		addComponent(_physics);
	}

}
