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
#include "Strings.hpp"
#include "Xml.hpp"

using namespace core;

namespace player {

	namespace {

		void BeamProjectile_SegmentQueryFunc(cpShape *shape, cpVect point, cpVect normal, cpFloat alpha, void *data) {
			if (cpShapeGetCollisionType( shape ) != CollisionType::PLAYER) {
				auto contacts = static_cast<vector<BeamProjectileComponent::contact>*>(data);

				// TODO: Figure out the game object associated with this shape. We need a convention?
				// cpShapeGetUserData is NOT used consistently. Terrain assigns shape user data to Anchor/Shape

				contacts->push_back(BeamProjectileComponent::contact(v2(point), v2(normal), nullptr));
			}
		}

		void BeamProjectile_MidpointQueryFunc(cpShape *shape, cpVect point, cpFloat distance, cpVect gradient, void *data) {
			if (cpShapeGetCollisionType( shape ) != CollisionType::PLAYER) {
				bool *hit = static_cast<bool*>(data);
				*hit = true;
			}
		}

	}

#pragma mark - BeamProjectileComponent

	/*
		config _config;
		dvec2 _origin, _dir, _position;
		double _life;
		seconds_t _birthSeconds, _lastDeltaT;
		mutable size_t _lastContactCalcTimestep;
		mutable vector<contact> _contacts;
	 */

	BeamProjectileComponent::BeamProjectileComponent(config c):
	_config(c),
	_origin(0),
	_dir(0),
	_position(0),
	_life(0),
	_birthSeconds(0),
	_lastDeltaT(0),
	_lastContactCalcTimestep(0)
	{}

	BeamProjectileComponent::BeamProjectileComponent(config c, dvec2 origin, dvec2 dir):
	_config(c),
	_origin(0),
	_dir(0),
	_position(0),
	_life(0),
	_birthSeconds(0),
	_lastDeltaT(0),
	_lastContactCalcTimestep(0)
	{
		fire(origin, dir);
	}

	BeamProjectileComponent::~BeamProjectileComponent(){}

	void BeamProjectileComponent::fire(dvec2 origin, dvec2 dir) {
		_origin = _position = origin;
		_dir = normalize(dir);
	}

	dvec2 BeamProjectileComponent::getPosition() const {
		return _position;
	}

	double BeamProjectileComponent::getWidth() const {
		return _config.width;
	}

	void BeamProjectileComponent::getSegment(dvec2 &a, dvec2 &b, double &len) {
		// TODO: Take into account collisions/contacts
		len = _lastDeltaT * _config.velocity;
		a = _position - _dir * len;
		b = _position + _dir * len;
	}

	// returns a vector of coordinates in world space representing the intersection with world geometry of the gun beam
	const vector<BeamProjectileComponent::contact> &BeamProjectileComponent::getContacts() const {
		//
		// this is fairly expensive, so we only want to calculate it once per timestep if it's requested repeatedly
		//

		size_t step = getLevel()->getTime().step;
		if (step > _lastContactCalcTimestep) {

			_lastContactCalcTimestep = step;
			_contacts.clear();

			//
			//	Collect all contacts
			//

			const dvec2 origin = _origin;

			cpSpace *space = getLevel()->getSpace()->getSpace();
			cpVect start = cpv(_origin);
			cpVect end = cpv(_origin + (_config.range * _dir));
			double radius = _config.width / 2;

			vector<contact> rawContacts;
			cpSpaceSegmentQuery(space, start, end, radius, CP_SHAPE_FILTER_ALL, BeamProjectile_SegmentQueryFunc, &rawContacts);
			cpSpaceSegmentQuery(space, end, start, radius, CP_SHAPE_FILTER_ALL, BeamProjectile_SegmentQueryFunc, &rawContacts);

			//
			// sort contacts by distance from gun
			//

			std::sort(rawContacts.begin(), rawContacts.end(), [&origin](const contact &a, const contact &b){
				return distanceSquared(origin, a.position) < distanceSquared(origin, b.position);
			});

			vector<contact> dedupedContacts;
			if (rawContacts.size() > 1) {

				//
				//	Dedup
				//

				dedupedContacts.reserve(rawContacts.size());
				dedupedContacts.push_back(rawContacts.front());
				const double dedupDistanceThreshold2 = 0.1 * 0.1;

				dvec2 lastContactPosition = rawContacts.front().position;
				for (auto it = rawContacts.begin()+1, end = rawContacts.end(); it != end; ++it) {
					if (distanceSquared(lastContactPosition, it->position) > dedupDistanceThreshold2) {
						dedupedContacts.push_back(*it);
					}
					lastContactPosition = it->position;
				}
			}

			//
			//	Now run a point query at midpoint of each segment. We want to discard inter-shape terrain contacts
			//

			if (dedupedContacts.size() > 2) {

				//
				//	TODO: This can be optimized, running halfway queries as we go, instead of building a vector<bool>
				//

				vector<int> midpointState; // I hear vector<bool> has big problems
				midpointState.reserve(dedupedContacts.size());

				dvec2 a = origin;
				for (auto it = dedupedContacts.begin(), end = dedupedContacts.end(); it != end; ++it) {
					dvec2 b = it->position;
					dvec2 mid = (a+b) * 0.5;

					bool hit = false;
					cpSpacePointQuery(space, cpv(mid), 1e-3, CP_SHAPE_FILTER_ALL, BeamProjectile_MidpointQueryFunc, &hit);
					midpointState.push_back(hit ? 1 : 0);

					a = b;
				}

				// final query to end of beam
				dvec2 mid = (a + v2(end)) * 0.5;
				bool hit = false;
				cpSpacePointQuery(space, cpv(mid), 1e-3, CP_SHAPE_FILTER_ALL, BeamProjectile_MidpointQueryFunc, &hit);
				midpointState.push_back(hit);

				// now add only those contacts which represent an edge between occupied space and empty space
				for (size_t i = 0, N = dedupedContacts.size(); i < N; i++) {
					if (midpointState[i] != midpointState[i+1]) {
						_contacts.push_back(dedupedContacts[i]);
					}
				}

			} else {
				_contacts = dedupedContacts;
			}
		}

		return _contacts;
	}

	void BeamProjectileComponent::onReady(GameObjectRef parent, LevelRef level) {
		_birthSeconds = time_state::now();
	}

	void BeamProjectileComponent::update(const time_state &time) {
		_lastDeltaT = time.deltaT;
		_life = (time.time - _birthSeconds) / _config.lifetime;
		_position = _position + _dir * _config.velocity * time.deltaT;

		GameObjectRef go = getGameObject();
		if (go) {
			if (_life > 1.0) {
				go->setFinished();
			} else {
				go->getDrawComponent()->notifyMoved();
			}
		}
	}

#pragma mark - BeamProjectileDrawComponent

	BeamProjectileDrawComponent::BeamProjectileDrawComponent(){}
	BeamProjectileDrawComponent::~BeamProjectileDrawComponent(){}

	void BeamProjectileDrawComponent::onReady(GameObjectRef parent, LevelRef level) {
		DrawComponent::onReady(parent, level);
		_projectile = getSibling<BeamProjectileComponent>();
	}

	cpBB BeamProjectileDrawComponent::getBB() const {
		if (BeamProjectileComponentRef projectile = _projectile.lock()) {
			dvec2 a,b;
			double len;
			projectile->getSegment(a, b, len);
			return cpBBNewLineSegment(a, b);
		}
		return cpBBInvalid;
	}

	void BeamProjectileDrawComponent::draw(const render_state &renderState) {
		if (BeamProjectileComponentRef projectile = _projectile.lock()) {
			dvec2 a,b;
			double len;
			projectile->getSegment(a, b, len);

			// scale width up to not be less than 1px
			double radius = projectile->getWidth() / 2;
			radius = max(radius, 0.5 / renderState.viewport->getScale());

			dvec2 dir = projectile->getDirection();
			dvec2 center = (a + b) * 0.5;
			double angle = atan2(dir.y, dir.x);

			dmat4 M = translate(dvec3(center, 0)) * rotate(angle, dvec3(0,0,1));
			gl::ScopedModelMatrix smm;
			gl::multModelMatrix(M);

			gl::color(0,1,1);
			gl::drawSolidRect(Rectf(-len/2, -radius, +len/2, +radius));
		}
	}

	VisibilityDetermination::style BeamProjectileDrawComponent::getVisibilityDetermination() const {
		return VisibilityDetermination::FRUSTUM_CULLING;
	}

	int BeamProjectileDrawComponent::getLayer() const {
		return DrawLayers::PLAYER;
	}

	int BeamProjectileDrawComponent::getDrawPasses() const {
		return 1;
	}



#pragma mark - PlayerGunComponent


	/*
		config _config;
		bool _isShooting;
		dvec2 _beamOrigin, _beamDir;
		double _blastCharge;
		seconds_t _pulseStartTime, _blastStartTime;
	 */

	PlayerGunComponent::PlayerGunComponent(config c):
	_config(c),
	_isShooting(false),
	_beamOrigin(0),
	_beamDir(0),
	_blastCharge(0),
	_pulseStartTime(0)
	{}

	PlayerGunComponent::~PlayerGunComponent() {}

	void PlayerGunComponent::setShooting(bool shooting) {

		if (shooting && !_isShooting) {
			firePulse();
		}

		if (_isShooting && !shooting) {
			if (_blastCharge >= 1) {
				fireBlast();
			}
		}

		_isShooting = shooting;
	}

	void PlayerGunComponent::update(const time_state &time) {

		if (_isShooting) {
			if (time.time - _pulseStartTime < _config.pulse.lifetime) {
				_blastCharge = 0;
			} else {
				_blastCharge = min(_blastCharge + time.deltaT * _config.blastChargePerSecond, 1.0);
			}
		} else {

			// if done shooting, reduce blast charge to zero
			seconds_t blastDur = time_state::now() - _blastStartTime;
			_blastCharge = max(1 - (blastDur / _config.blast.lifetime), 0.0);
		}

	}

	void PlayerGunComponent::firePulse() {
		_blastCharge = 0;
		_pulseStartTime = time_state::now();

		auto pulse = GameObject::with("Pulse", {
			make_shared<BeamProjectileComponent>(_config.pulse, getBeamOrigin(), getBeamDirection()),
			make_shared<BeamProjectileDrawComponent>()
		});

		getLevel()->addGameObject(pulse);

	}

	void PlayerGunComponent::fireBlast() {
		_blastStartTime = time_state::now();

		auto blast = GameObject::with("Blast", {
			make_shared<BeamProjectileComponent>(_config.blast, getBeamOrigin(), getBeamDirection()),
			make_shared<BeamProjectileDrawComponent>()
		});

		getLevel()->addGameObject(blast);
	}

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

	void PlayerPhysicsComponent::onReady(GameObjectRef parent, LevelRef level) {
		PhysicsComponent::onReady(parent, level);
	}

	dvec2 PlayerPhysicsComponent::_getGroundNormal() const
	{
		const double
			HalfWidth = getConfig().width * 0.5,
			SegmentRadius = HalfWidth * 0.125,
			CastDistance = 10000;

		const dvec2 Position(v2(cpBodyGetPosition(getFootBody())));

		const auto G = getSpace()->getGravity(Position);

		const dvec2
			Down = G.dir,
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
		double _jetpackFuelLevel, _jetpackFuelMax, _lean;
		dvec2 _up, _groundNormal;
	 */
	JetpackUnicyclePlayerPhysicsComponent::JetpackUnicyclePlayerPhysicsComponent(config c):
	PlayerPhysicsComponent(c),
	_body(nullptr),
	_wheelBody(nullptr),
	_bodyShape(nullptr),
	_wheelShape(nullptr),
	_groundContactSensorShape(nullptr),
	_wheelMotor(nullptr),
	_orientationConstraint(nullptr),
	_wheelRadius(0),
	_wheelFriction(0),
	_touchingGroundAcc(0),
	_jetpackFuelLevel(0),
	_jetpackFuelMax(0),
	_lean(0),
	_up(0,0),
	_groundNormal(0,0)
	{}

	JetpackUnicyclePlayerPhysicsComponent::~JetpackUnicyclePlayerPhysicsComponent()
	{}

	// PhysicsComponent
	void JetpackUnicyclePlayerPhysicsComponent::onReady(GameObjectRef parent, LevelRef level) {
		PlayerPhysicsComponent::onReady(parent, level);

		_input = getSibling<PlayerInputComponent>();

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
		// lozenge shape for body and gear joint to orient it
		//

		_bodyShape = _add(cpSegmentShapeNew( _body, cpv(0,-(HalfHeight)), cpv(0,HalfHeight), Width/2 ));
		cpShapeSetFriction( _bodyShape, config.bodyFriction );

		_orientationConstraint = _add(cpGearJointNew(cpSpaceGetStaticBody(getSpace()->getSpace()), _body, 0, 1));

		//
		// make wheel at bottom of body
		//

		_wheelRadius = WheelRadius;
		_wheelBody = _add(cpBodyNew( WheelMass, cpMomentForCircle( WheelMass, 0, _wheelRadius, cpvzero )));
		cpBodySetPosition( _wheelBody, cpvadd( cpBodyGetPosition( _body ), WheelPositionOffset ));
		_wheelShape = _add(cpCircleShapeNew( _wheelBody, _wheelRadius, cpvzero ));
		cpShapeSetFriction(_wheelShape, config.footFriction);
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

	void JetpackUnicyclePlayerPhysicsComponent::step(const time_state &timeState) {
		PlayerPhysicsComponent::step(timeState);

		const PlayerRef player = getGameObjectAs<Player>();
		const PlayerPhysicsComponent::config config = getConfig();

		const bool
			Alive = true, //player->iaAlive(),
			Restrained = false; // player->isRestrained();

		const double
			Vel = getSpeed(),
			Dir = sign( getSpeed() );

		const auto G = getSpace()->getGravity(v2(cpBodyGetPosition(_wheelBody)));

		const dvec2
			Down = G.dir,
			Right = rotateCCW(Down);

		//
		//	Smoothly update touching ground state as well as ground normal
		//

		_touchingGroundAcc = lrp<double>(0.2,_touchingGroundAcc,_isTouchingGround(_groundContactSensorShape) ? 1.0 : 0.0);
		_groundNormal = normalize( lrp( 0.2, _groundNormal, _getGroundNormal()));

		{
			const dvec2 ActualUp = -Down;
			_up = normalize(lrp( 0.2, _up, ActualUp));

			double target = std::atan2( _up.y, _up.x ) - M_PI_2;
			double phase = cpGearJointGetPhase(_orientationConstraint);
			double turn = closest_turn_radians(phase, target);
			double newPhase = phase + turn;

			// add lean
			_lean = lrp<double>(0.2, _lean, Dir);
			newPhase -= _lean * M_PI * 0.125;

			cpGearJointSetPhase(_orientationConstraint, newPhase);
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
			dvec2 antiGravForceDir = normalize((0.5 * Dir * Right) + G.dir);
			dvec2 force = -config.jetpackAntigravity * _totalMass * G.force * antiGravForceDir;
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
				FootFriction = getConfig().footFriction,
				LockdownFriction = 1000 * FootFriction,
				Stillness = (1-abs( sign( Vel ))),
				WheelFriction = lrp<double>( Stillness, FootFriction, LockdownFriction );

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
			cpConstraintSetMaxForce( _orientationConstraint, 0 );
			cpConstraintSetMaxBias( _orientationConstraint, 0 );
			cpConstraintSetErrorBias( _orientationConstraint, 0 );
			for (cpShape *shape : getShapes()) {
				cpShapeSetFriction(shape, 0);
			}
		}
		else
		{
			// orientation constraint may have been lessened when player was crushed, we always want to ramp it back up
			const double MaxForce = cpConstraintGetMaxForce(_orientationConstraint);
			if ( MaxForce < 10000 )
			{
				cpConstraintSetMaxForce( _orientationConstraint, max(MaxForce,10.0) * 1.1 );
			}
			else
			{
				cpConstraintSetMaxForce( _orientationConstraint, INFINITY );
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

	bool PlayerInputComponent::isShooting() const {
		return isMouseLeftButtonDown();
	}

	dvec2 PlayerInputComponent::getShootingTargetWorld() const {
		return getLevel()->getViewport()->screenToWorld(getMousePosition());
	}

#pragma mark - PlayerDrawComponent

	PlayerDrawComponent::PlayerDrawComponent()
	{}

	PlayerDrawComponent::~PlayerDrawComponent()
	{}

	void PlayerDrawComponent::onReady(GameObjectRef parent, LevelRef level) {
		DrawComponent::onReady(parent, level);
		_physics = getSibling<JetpackUnicyclePlayerPhysicsComponent>();
		_gun = getSibling<PlayerGunComponent>();
	}

	cpBB PlayerDrawComponent::getBB() const {
		if (PlayerPhysicsComponentRef ppc = _physics.lock()) {
			return ppc->getBB();
		}

		return cpBBZero;
	}

	void PlayerDrawComponent::draw(const render_state &renderState) {
		drawPlayer(renderState);
	}

	void PlayerDrawComponent::drawScreen(const render_state &renderState) {
		PlayerGunComponentRef gun = _gun.lock();
		assert(gun);
		drawGunCharge(gun, renderState);
	}

	VisibilityDetermination::style PlayerDrawComponent::getVisibilityDetermination() const {
		return VisibilityDetermination::FRUSTUM_CULLING;
	}

	int PlayerDrawComponent::getLayer() const {
		return DrawLayers::PLAYER;
	}

	int PlayerDrawComponent::getDrawPasses() const {
		return 1;
	}

	void PlayerDrawComponent::drawPlayer(const render_state &renderState) {
		JetpackUnicyclePlayerPhysicsComponentRef physics = _physics.lock();
		assert(physics);

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

	void PlayerDrawComponent::drawGunCharge(PlayerGunComponentRef gun, const render_state &renderState) {
		// we're in screen space
		Rectd bounds = renderState.viewport->getBounds();

		int w = 20;
		int h = 60;
		int p = 10;
		Rectf chargeRectFrame(bounds.getWidth() - p, p, bounds.getWidth() - p - w, p + h);

		gl::color(0,1,1,1);
		gl::drawStrokedRect(chargeRectFrame);

		double charge = sqrt(gun->getBlastChargeLevel());
		int fillHeight = static_cast<int>(ceil(charge * h));
		Rectf chargeRectFill(bounds.getWidth() - p, p + h-fillHeight, bounds.getWidth() - p - w, p + h);
		gl::drawSolidRect(chargeRectFill);
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

		//
		//	Gun
		//

		XmlTree gunNode = playerNode.getChild("gun");
		config.gun.blastChargePerSecond = util::xml::readNumericAttribute(gunNode, "blastChargePerSecond", 0.3);

		XmlTree pulseNode = gunNode.getChild("pulse");
		config.gun.pulse.range = util::xml::readNumericAttribute(pulseNode, "range", 1000);
		config.gun.pulse.width = util::xml::readNumericAttribute(pulseNode, "width", 2);
		config.gun.pulse.velocity = util::xml::readNumericAttribute(pulseNode, "velocity", 100);
		config.gun.pulse.lifetime = util::xml::readNumericAttribute(pulseNode, "lifetime", 1);
		config.gun.pulse.cutDepth = 0;

		XmlTree blastNode = gunNode.getChild("blast");
		config.gun.blast.range = util::xml::readNumericAttribute(blastNode, "range", 1000);
		config.gun.blast.width = util::xml::readNumericAttribute(blastNode, "width", 2);
		config.gun.blast.velocity = util::xml::readNumericAttribute(blastNode, "velocity", 100);
		config.gun.blast.lifetime = util::xml::readNumericAttribute(blastNode, "lifetime", 1);
		config.gun.blast.cutDepth = util::xml::readNumericAttribute(blastNode, "cutDepth", 300);

		//
		//	Physics
		//

		XmlTree physicsNode = playerNode.getChild("physics");

		config.physics.position = position;
		config.physics.width = util::xml::readNumericAttribute(physicsNode, "width", 5);
		config.physics.height = util::xml::readNumericAttribute(physicsNode, "height", 20);
		config.physics.density = util::xml::readNumericAttribute(physicsNode, "density", 1);
		config.physics.footFriction = util::xml::readNumericAttribute(physicsNode, "footFriction", 1);
		config.physics.bodyFriction = util::xml::readNumericAttribute(physicsNode, "bodyFriction", 0.2);

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
		GameObject::update(time);

		//
		//	Synchronize
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


			//
			//	Compute gun origin and direction
			//	TODO: Be smarter about gun origin
			//

			_gun->setBeamOrigin((_physics->getPosition()));
			_gun->setBeamDirection(normalize(_input->getShootingTargetWorld() - _gun->getBeamOrigin()));
			_gun->setShooting(_input->isShooting());
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
		_gun = make_shared<PlayerGunComponent>(c.gun);

		addComponent(_input);
		addComponent(_drawing);
		addComponent(_physics);
		addComponent(_gun);
	}

}
