//
//  Player.hpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 4/23/17.
//
//

#ifndef Player_hpp
#define Player_hpp

#include <cinder/Xml.h>

#include "Core.hpp"
#include "DevComponents.hpp"

namespace player {

	SMART_PTR(Player);
	SMART_PTR(PlayerPhysicsComponent);
	SMART_PTR(JetpackUnicyclePlayerPhysicsComponent);
	SMART_PTR(PlayerDrawComponent);
	SMART_PTR(PlayerInputComponent);

#pragma mark - Player Components

	class PlayerPhysicsComponent : public core::PhysicsComponent {
	public:

		struct config {
			// initial position of player in world units
			dvec2 position;

			double width;
			double height;
			double density;
			double friction;

			double jetpackAntigravity;
			double jetpackFuelMax;
			double jetpackFuelConsumptionPerSecond;
			double jetpackFuelRegenerationPerSecond;
		};

	public:

		PlayerPhysicsComponent(config c);
		virtual ~PlayerPhysicsComponent();

		// PhysicsComponent
		void onReady(core::GameObjectRef parent, core::LevelRef level) override;
		virtual vector<cpBody*> getBodies() const override { return _bodies; }

		// PlayerPhysicsComponent
		const config& getConfig()const { return _config; }

		virtual cpBB getBB() const = 0;
		virtual dvec2 getPosition() const = 0;
		virtual dvec2 getUp() const = 0;
		virtual dvec2 getGroundNormal() const = 0;
		virtual bool isTouchingGround() const = 0;
		virtual cpBody *getBody() const = 0;
		virtual cpBody *getFootBody() const = 0;
		virtual double getJetpackFuelLevel() const = 0;
		virtual double getJetpackFuelMax() const = 0;

		vector<cpShape*> getShapes() const { return _shapes; }
		vector<cpConstraint*> getConstraints() const { return _constraints; }

		// Control inputs, called by Player in Player::update

		virtual void setSpeed( double vel ) { _speed = vel; }
		double getSpeed() const { return _speed; }

		virtual void setFlying( bool j ) { _flying = j; }
		bool isFlying() const { return _flying; }

	protected:

		cpShape *_add( cpShape *shape ) { _shapes.push_back(shape); return shape; }
		cpConstraint *_add( cpConstraint *constraint ) { _constraints.push_back(constraint); return constraint; }
		cpBody *_add( cpBody *body ) { _bodies.push_back(body); return body; }

		dvec2 _getGroundNormal() const;
		bool _isTouchingGround( cpShape *shape ) const;

	protected:

		config _config;
		vector<cpBody*> _bodies;
		vector<cpShape*> _shapes;
		vector<cpConstraint*> _constraints;
		bool _flying;
		double _speed;

	};

	class JetpackUnicyclePlayerPhysicsComponent : public PlayerPhysicsComponent {
	public:

		JetpackUnicyclePlayerPhysicsComponent(config c);
		virtual ~JetpackUnicyclePlayerPhysicsComponent();

		// PhysicsComponent
		void onReady(core::GameObjectRef parent, core::LevelRef level) override;
		void step(const core::time_state &timeState) override;

		// PlayerPhysicsComponent
		cpBB getBB() const override;
		dvec2 getPosition() const override;
		dvec2 getUp() const override;
		dvec2 getGroundNormal() const override;
		bool isTouchingGround() const override;
		cpBody *getBody() const override;
		cpBody *getFootBody() const override;
		double getJetpackFuelLevel() const override;
		double getJetpackFuelMax() const override;

		//PogoCyclePlayerPhysicsComponent

		struct capsule {
			dvec2 a,b;
			double radius;
		};

		capsule getBodyCapsule() const;

		struct wheel {
			dvec2 position;
			double radius;
			double radians;
		};

		wheel getFootWheel() const;


	private:

		cpBody *_body, *_wheelBody;
		cpShape *_bodyShape, *_wheelShape, *_groundContactSensorShape;
		cpConstraint *_wheelMotor, *_orientationConstraint;
		double _wheelRadius, _wheelFriction, _touchingGroundAcc, _totalMass;
		double _jetpackFuelLevel, _jetpackFuelMax, _lean;
		dvec2 _up, _groundNormal;

	};

	class PlayerInputComponent : public core::InputComponent {
	public:

		PlayerInputComponent();
		virtual ~PlayerInputComponent();

		// actions
		bool isRunning() const;
		bool isGoingRight() const;
		bool isGoingLeft() const;
		bool isJumping() const;
		bool isCrouching() const;

	private:

		int _keyRun, _keyLeft, _keyRight, _keyJump, _keyCrouch;

	};

	class PlayerDrawComponent : public core::DrawComponent {
	public:

		PlayerDrawComponent();
		virtual ~PlayerDrawComponent();

		void onReady(core::GameObjectRef parent, core::LevelRef level) override;

		cpBB getBB() const override;
		void draw(const core::render_state &renderState) override;
		core::VisibilityDetermination::style getVisibilityDetermination() const override;
		int getLayer() const override;
		int getDrawPasses() const override;
		BatchDrawDelegateRef getBatchDrawDelegate() const override { return nullptr; }
		
	private:
		
		JetpackUnicyclePlayerPhysicsComponentWeakRef _physics;
		
	};

#pragma mark - Player

	class Player : public core::GameObject, public TargetTrackingViewportControlComponent::TrackingTarget {
	public:

		struct config {
			PlayerPhysicsComponent::config physics;
			// TODO: Add a PlayerDrawComponent::config for appearance control

			double walkSpeed;
			double runMultiplier;

			config():
			walkSpeed(1), // 1mps
			runMultiplier(3)
			{}
		};

		/**
		 Create a player configured via the XML in playerXmlFile, at a given position in world units
		 */
		static PlayerRef create(string name, ci::DataSourceRef playerXmlFile, dvec2 position);

	public:
		Player(string name);
		virtual ~Player();

		const config &getConfig() const { return _config; }

		// GameObject
		void update(const core::time_state &time) override;

		// TrackingTarget
		TargetTrackingViewportControlComponent::tracking getViewportTracking() const override;

	protected:

		virtual void build(config c);

	private:

		config _config;
		PlayerPhysicsComponentRef _physics;
		PlayerDrawComponentRef _drawing;
		PlayerInputComponentRef _input;

	};

}

#endif /* Player_hpp */
