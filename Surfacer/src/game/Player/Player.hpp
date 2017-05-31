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
#include "Entity.hpp"
#include "DevComponents.hpp"

namespace core { namespace game { namespace player {

	SMART_PTR(Player);
	SMART_PTR(PlayerGunComponent);
	SMART_PTR(PlayerPhysicsComponent);
	SMART_PTR(JetpackUnicyclePlayerPhysicsComponent);
	SMART_PTR(PlayerDrawComponent);
	SMART_PTR(PlayerInputComponent);
	SMART_PTR(BeamComponent);

#pragma mark - BeamProjectileComponent

	class BeamComponent : public Component {
	public:

		struct config {
			double width;
			double range;

			config():
			width(0),
			range(0)
			{}
		};

		struct contact {
			dvec2 position;
			dvec2 normal;
			GameObjectRef target;
		};

		struct segment {
			dvec2 head, tail;
			double len;

			segment():
			head(0),
			tail(0),
			len(0)
			{}
		};

	public:

		BeamComponent(config c, PlayerRef player);
		virtual ~BeamComponent();

		virtual void fire(dvec2 origin, dvec2 dir);
		dvec2 getOrigin() const { return _origin; }
		dvec2 getDirection() const { return _dir; }
		double getWidth() const;
		segment getSegment() const { return _segment; }
		PlayerRef getPlayer() const { return _player.lock(); }

		// returns a vector of coordinates in world space representing the intersection with world geometry of the gun beam
		const vector<contact> &getContacts() const { return _contacts; }

		// Component
		void onReady(GameObjectRef parent, LevelRef level) override;
		void update(const time_state &time) override;

	protected:

		void updateContacts();

	protected:

		config _config;
		PlayerWeakRef _player;
		dvec2 _origin, _dir;
		segment _segment;
		vector<contact> _contacts;

	};

	class PulseBeamComponent : public BeamComponent {
	public:

		struct config : public BeamComponent::config {
			double length;
			double velocity;

			config():
			length(0),
			velocity(0)
			{}
		};

	public:

		PulseBeamComponent(config c, PlayerRef player);
		virtual ~PulseBeamComponent();

		// Component
		void update(const time_state &time) override;

	protected:

		config _config;
		double _distanceTraveled;
		bool _hasHit;

	};

	class BlastBeamComponent : public BeamComponent {
	public:

		struct config : public BeamComponent::config {
			seconds_t lifespan;
			double cutDepth;

			config():
			lifespan(0),
			cutDepth(0)
			{}
		};

	public:

		BlastBeamComponent(config c, PlayerRef player);
		virtual ~BlastBeamComponent(){}

		// BeamComponent
		void fire(dvec2 origin, dvec2 dir) override;

		// Component
		void onReady(GameObjectRef parent, LevelRef level) override;
		void update(const time_state &time) override;

	protected:

		void computeCutSegment();

	protected:

		config _config;
		seconds_t _startSeconds;

	};

#pragma mark - BeamDrawComponent

	class BeamDrawComponent : public DrawComponent {
	public:

		BeamDrawComponent();
		virtual ~BeamDrawComponent();

		// Component
		void onReady(GameObjectRef parent, LevelRef level) override;

		// DrawComponent
		cpBB getBB() const override;
		void draw(const render_state &renderState) override;
		VisibilityDetermination::style getVisibilityDetermination() const override;
		int getLayer() const override;
		int getDrawPasses() const override;
		BatchDrawDelegateRef getBatchDrawDelegate() const override { return nullptr; }

	private:

		BeamComponentWeakRef _beam;

	};


#pragma mark - PlayerGunComponent

	class PlayerGunComponent : public Component {
	public:

		struct config {
			PulseBeamComponent::config pulse;
			BlastBeamComponent::config blast;
			double blastChargePerSecond;
		};

	public:

		PlayerGunComponent(config c);
		virtual ~PlayerGunComponent();

		void setShooting(bool shooting);
		bool isShooting() const { return _shooting; }

		// get the current charge level [0,1]
		double getBlastChargeLevel() const { return _blastCharge; }

		// origin of gun beam in world space
		void setBeamOrigin(dvec2 origin) { _beamOrigin = origin; }
		dvec2 getBeamOrigin() const { return _beamOrigin; }

		// normalized direction of gun beam in world space
		void setBeamDirection(dvec2 dir) { _beamDir = dir; }
		dvec2 getBeamDirection() const { return _beamDir; }

		// Component
		void update(const time_state &time) override;

	private:

		void firePulse();
		void fireBlast();

	private:

		config _config;
		bool _shooting;
		dvec2 _beamOrigin, _beamDir;
		double _blastCharge;
		seconds_t _pulseStartTime;

	};

#pragma mark - PlayerPhysicsComponent

	class PlayerPhysicsComponent : public PhysicsComponent {
	public:

		struct config {
			// initial position of player in world units
			dvec2 position;

			double width;
			double height;
			double density;
			double footFriction;
			double bodyFriction;

			double jetpackAntigravity;
			double jetpackFuelMax;
			double jetpackFuelConsumptionPerSecond;
			double jetpackFuelRegenerationPerSecond;
		};

	public:

		PlayerPhysicsComponent(config c);
		virtual ~PlayerPhysicsComponent();

		// PhysicsComponent
		void onReady(GameObjectRef parent, LevelRef level) override;

		// PlayerPhysicsComponent
		const config& getConfig()const { return _config; }

		virtual dvec2 getPosition() const = 0;
		virtual dvec2 getUp() const = 0;
		virtual dvec2 getGroundNormal() const = 0;
		virtual bool isTouchingGround() const = 0;
		virtual cpBody *getBody() const = 0;
		virtual cpBody *getFootBody() const = 0;
		virtual cpShape *getBodyShape() const = 0;
		virtual cpShape *getFootShape() const = 0;
		virtual double getJetpackFuelLevel() const = 0;
		virtual double getJetpackFuelMax() const = 0;
		virtual dvec2 getJetpackThrustDirection() const = 0;

		// Control inputs, called by Player in Player::update
		virtual void setSpeed( double vel ) { _speed = vel; }
		double getSpeed() const { return _speed; }

		virtual void setFlying( bool j ) { _flying = j; }
		bool isFlying() const { return _flying; }

	protected:

		dvec2 _getGroundNormal() const;
		bool _isTouchingGround( cpShape *shape ) const;


	protected:

		config _config;
		bool _flying;
		double _speed;

	};

#pragma mark - JetpackUnicyclePlayerPhysicsComponent

	class JetpackUnicyclePlayerPhysicsComponent : public PlayerPhysicsComponent {
	public:

		JetpackUnicyclePlayerPhysicsComponent(config c);
		virtual ~JetpackUnicyclePlayerPhysicsComponent();

		// PhysicsComponent
		cpBB getBB() const override;
		void onReady(GameObjectRef parent, LevelRef level) override;
		void step(const time_state &timeState) override;

		// PlayerPhysicsComponent
		dvec2 getPosition() const override;
		dvec2 getUp() const override;
		dvec2 getGroundNormal() const override;
		bool isTouchingGround() const override;
		cpBody *getBody() const override;
		cpBody *getFootBody() const override;
		cpShape *getBodyShape() const override;
		cpShape *getFootShape() const override;
		double getJetpackFuelLevel() const override;
		double getJetpackFuelMax() const override;
		dvec2 getJetpackThrustDirection() const override;

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
		dvec2 _up, _groundNormal, _jetpackForceDir;
		PlayerInputComponentWeakRef _input;
	};

#pragma mark - PlayerInputComponent

	class PlayerInputComponent : public InputComponent {
	public:

		PlayerInputComponent();
		virtual ~PlayerInputComponent();

		// actions
		bool isRunning() const;
		bool isGoingRight() const;
		bool isGoingLeft() const;
		bool isJumping() const;
		bool isCrouching() const;
		bool isShooting() const;
		dvec2 getShootingTargetWorld() const;

	private:

		int _keyRun, _keyLeft, _keyRight, _keyJump, _keyCrouch;
		bool _shooting;

	};

#pragma mark - PlayerDrawComponent

	class PlayerDrawComponent : public DrawComponent {
	public:

		PlayerDrawComponent();
		virtual ~PlayerDrawComponent();

		// DrawComponent
		void onReady(GameObjectRef parent, LevelRef level) override;

		cpBB getBB() const override;
		void draw(const render_state &renderState) override;
		void drawScreen(const render_state &renderState) override;
		VisibilityDetermination::style getVisibilityDetermination() const override;
		int getLayer() const override;
		int getDrawPasses() const override;
		BatchDrawDelegateRef getBatchDrawDelegate() const override { return nullptr; }

	protected:

		void drawPlayer(const render_state &renderState);
		void drawGunCharge(PlayerGunComponentRef gun, const render_state &renderState);

	private:
		
		JetpackUnicyclePlayerPhysicsComponentWeakRef _physics;
		PlayerGunComponentWeakRef _gun;
		
	};

#pragma mark - Player

	class Player : public Entity, public TargetTrackingViewportControlComponent::TrackingTarget {
	public:

		signals::signal< void( const BeamComponentRef &, const BeamComponent::contact &contact ) > didShootSomething;

		struct config {
			PlayerPhysicsComponent::config physics;
			PlayerGunComponent::config gun;
			HealthComponent::config health;

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

		// Entity
		void onHealthChanged(double oldHealth, double newHealth) override;
		void onDeath() override;

		// GameObject
		void update(const time_state &time) override;

		// TrackingTarget
		TargetTrackingViewportControlComponent::tracking getViewportTracking() const override;

		const PlayerPhysicsComponentRef &getPhysics() const { return _physics; }
		const PlayerInputComponentRef &getInput() const { return _input; }
		const PlayerGunComponentRef &getGun() const { return _gun; }

	protected:

		friend class BeamComponent;
		virtual void onShotSomething(const BeamComponentRef &beam, const BeamComponent::contact &contact);

		virtual void build(config c);

	private:

		config _config;
		PlayerPhysicsComponentRef _physics;
		PlayerDrawComponentRef _drawing;
		PlayerInputComponentRef _input;
		PlayerGunComponentRef _gun;

	};

}}} // namespace core::game::player

#endif /* Player_hpp */
