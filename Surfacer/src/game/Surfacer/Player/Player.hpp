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

namespace surfacer { namespace player {

	SMART_PTR(Player);
	SMART_PTR(Gun);
	SMART_PTR(PlayerPhysicsComponent);
	SMART_PTR(JetpackUnicyclePlayerPhysicsComponent);
	SMART_PTR(PlayerDrawComponent);
	SMART_PTR(PlayerInputComponent);
	SMART_PTR(Projectile);
	SMART_PTR(BeamProjectile);

#pragma mark - Projectile

	class Projectile : public core::Component {
	public:

		struct config {
			double range;
			double damage;

			config():
			range(0),
			damage(0)
			{}
		};

		struct contact {
			dvec2 position;
			dvec2 normal;
			cpShape *shape;
			core::ObjectRef object;

			contact():
			position(0),
			normal(0),
			shape(nullptr)
			{}

			contact(const contact &c):
			position(c.position),
			normal(c.normal),
			shape(c.shape),
			object(c.object)
			{}
		};

	public:

		Projectile(config c, PlayerRef player);
		virtual ~Projectile();

		virtual void fire(dvec2 origin, dvec2 dir);
		dvec2 getOrigin() const { return _origin; }
		dvec2 getDirection() const { return _dir; }
		PlayerRef getPlayer() const { return _player.lock(); }
		float getDamage() const { return _config.damage; }

		// returns a vector of coordinates in world space representing the intersection with world geometry of the gun beam
		const vector<contact> &getContacts() const { return _contacts; }

	protected:

		// call this to compute contacts between projectile and scene and notify level
		// note this is only critical for projectiles with synthetic collisions (e.g., lasers)
		// not for a rock or some other thing which can be added to the game space
		void processContacts();

		// custom implementations should compute their contacts here stuffing them into _contacts
		// which is empty when this function is called. the results will be handled by processContacts
		virtual void updateContacts() = 0;

	protected:

		config _config;
		PlayerWeakRef _player;
		dvec2 _origin, _dir;
		vector<contact> _contacts;

	};

	class BeamProjectile : public Projectile {
	public:
		struct config : public Projectile::config {
			float width;

			config():
			width(0)
			{}
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

		BeamProjectile(config c, PlayerRef player);
		virtual ~BeamProjectile();

		void fire(dvec2 origin, dvec2 dir) override;
		double getWidth() const { return _config.width; }
		segment getSegment() const { return _segment; }

	protected:

		void updateContacts() override;

	protected:

		config _config;
		segment _segment;

	};


	class PulseProjectile : public BeamProjectile {
	public:

		struct config : public BeamProjectile::config {
			double length;
			double velocity;

			config():
			length(0),
			velocity(0)
			{}
		};

	public:

		PulseProjectile(config c, PlayerRef player);
		virtual ~PulseProjectile();

		// Component
		void update(const core::time_state &time) override;

	protected:

		config _config;
		double _distanceTraveled;
		bool _hasHit;

	};

	class CutterProjectile : public BeamProjectile {
	public:

		struct config : public BeamProjectile::config {
			core::seconds_t lifespan;
			double cutDepth;

			config():
			lifespan(0),
			cutDepth(0)
			{}
		};

	public:

		CutterProjectile(config c, PlayerRef player);
		virtual ~CutterProjectile(){}

		// Projectile
		void fire(dvec2 origin, dvec2 dir) override;

		// Component
		void onReady(core::ObjectRef parent, core::LevelRef level) override;
		void update(const core::time_state &time) override;

	protected:

		void computeCutSegment();

	protected:

		config _config;
		core::seconds_t _startSeconds;

	};

#pragma mark - BeamProjectileDrawComponent

	class BeamProjectileDrawComponent : public core::DrawComponent {
	public:

		BeamProjectileDrawComponent();
		virtual ~BeamProjectileDrawComponent();

		// Component
		void onReady(core::ObjectRef parent, core::LevelRef level) override;

		// DrawComponent
		cpBB getBB() const override;
		void draw(const core::render_state &renderState) override;
		core::VisibilityDetermination::style getVisibilityDetermination() const override;
		int getLayer() const override;
		int getDrawPasses() const override;
		BatchDrawDelegateRef getBatchDrawDelegate() const override { return nullptr; }

	private:

		BeamProjectileWeakRef _beam;
		
	};


#pragma mark - Gun

	class Gun : public core::Component {
	public:

		struct config {
			PulseProjectile::config pulse;
			CutterProjectile::config cutter;
			double cutterChargePerSecond;
		};

	public:

		Gun(config c);
		virtual ~Gun();

		void setShooting(bool shooting);
		bool isShooting() const { return _shooting; }

		// get the current charge level [0,1]
		double getCharge() const { return _charge; }

		// origin of gun beam in world space
		void setAimOrigin(dvec2 origin) { _aimOrigin = origin; }
		dvec2 getAimOrigin() const { return _aimOrigin; }

		// normalized direction of gun beam in world space
		void setAimDirection(dvec2 dir) { _aimDir = dir; }
		dvec2 getAimDirection() const { return _aimDir; }

		// Component
		void update(const core::time_state &time) override;

	private:

		void firePulse();
		void fireCutter();

	private:

		config _config;
		bool _shooting;
		dvec2 _aimOrigin, _aimDir;
		double _charge;
		core::seconds_t _chargeStartTime;

	};

#pragma mark - PlayerPhysicsComponent

	class PlayerPhysicsComponent : public core::PhysicsComponent {
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
		void onReady(core::ObjectRef parent, core::LevelRef level) override;

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
		void onReady(core::ObjectRef parent, core::LevelRef level) override;
		void step(const core::time_state &timeState) override;

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
		bool isShooting() const;
		dvec2 getShootingTargetWorld() const;

	private:

		int _keyRun, _keyLeft, _keyRight, _keyJump, _keyCrouch;
		bool _shooting;

	};

#pragma mark - PlayerDrawComponent

	class PlayerDrawComponent : public core::EntityDrawComponent {
	public:

		PlayerDrawComponent();
		virtual ~PlayerDrawComponent();

		// DrawComponent
		void onReady(core::ObjectRef parent, core::LevelRef level) override;

		cpBB getBB() const override;
		void draw(const core::render_state &renderState) override;
		void drawScreen(const core::render_state &renderState) override;
		core::VisibilityDetermination::style getVisibilityDetermination() const override;
		int getLayer() const override;
		int getDrawPasses() const override;
		BatchDrawDelegateRef getBatchDrawDelegate() const override { return nullptr; }

	protected:

		void drawPlayer(const core::render_state &renderState);
		void drawGunCharge(GunRef gun, const core::render_state &renderState);

	private:
		
		JetpackUnicyclePlayerPhysicsComponentWeakRef _physics;
		GunWeakRef _gun;
		
	};

#pragma mark - Player

	class Player : public core::Entity, public TargetTrackingViewportControlComponent::TrackingTarget {
	public:

		struct config {
			PlayerPhysicsComponent::config physics;
			Gun::config gun;
			core::HealthComponent::config health;

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

		// Object
		void update(const core::time_state &time) override;

		// TrackingTarget
		TargetTrackingViewportControlComponent::tracking getViewportTracking() const override;

		const PlayerPhysicsComponentRef &getPhysics() const { return _physics; }
		const PlayerInputComponentRef &getInput() const { return _input; }
		const GunRef &getGun() const { return _gun; }

	protected:

		virtual void build(config c);

	private:

		config _config;
		PlayerPhysicsComponentRef _physics;
		PlayerDrawComponentRef _drawing;
		PlayerInputComponentRef _input;
		GunRef _gun;

	};

}} // namespace surfacer::player

#endif /* Player_hpp */
