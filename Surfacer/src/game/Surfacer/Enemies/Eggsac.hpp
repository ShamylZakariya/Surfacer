//
//  Eggsac.hpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 5/8/17.
//
//

#ifndef Eggsac_hpp
#define Eggsac_hpp

#include <cinder/Xml.h>

#include "Core.hpp"
#include "Entity.hpp"
#include "Boid.hpp"

#include "Xml.hpp"

namespace surfacer { namespace enemy {

	SMART_PTR(Eggsac);
	SMART_PTR(EggsacPhysicsComponent);
	SMART_PTR(EggsacDrawComponent);

	class EggsacDrawComponent : public core::EntityDrawComponent {
	public:

		struct config {
		};

	public:

		EggsacDrawComponent(config c);
		virtual ~EggsacDrawComponent();

		config getConfig() const { return _config; }

		// DrawComponent
		void onReady(core::ObjectRef parent, core::LevelRef level) override;

		void draw(const core::render_state &renderState) override;
		core::VisibilityDetermination::style getVisibilityDetermination() const override { return core::VisibilityDetermination::FRUSTUM_CULLING; }
		int getLayer() const override;
		int getDrawPasses() const override { return 1; }
		BatchDrawDelegateRef getBatchDrawDelegate() const override { return nullptr; }

	protected:

		config _config;
		EggsacPhysicsComponentWeakRef _physics;

	};

	class EggsacPhysicsComponent : public core::PhysicsComponent {
	public:

		struct config {
			dvec2 suggestedAttachmentPosition;
			double density;
			double width;
			double height;

			config():
			density(1),
			width(0),
			height(0)
			{}
		};

	public:

		EggsacPhysicsComponent(config c);
		virtual ~EggsacPhysicsComponent();

		config getConfig() const { return _config; }
		dvec2 getPosition() const { return v2(cpBodyGetPosition(_sacBody)); }
		dvec2 getUp() const { return _up; }
		dvec2 getRight() const { return _right; }
		double getHeight() const { return _config.height; }
		double getWidth() const { return _config.width; }

		cpBody*	getBody() const { return _sacBody; }
		cpShape* getBodyShape() const { return _sacShape; }
		cpConstraint *getAttachmentSpring() const { return _attachmentSpring; }

		// return true if eggsac should automatically attach to terrain when not currently attached
		bool shouldAutomaticallyAttach() const { return _automaticallyAttach; }
		void setShouldAutomaticallyAttach(bool a) { _automaticallyAttach = a; }

		// return true iff attached to terrain
		bool isAttached() const;

		// if !attached, attach to closest terrain
		void attach();

		// if attached, detach from whatever terrain attached to
		void detach();

		// PhysicsComponent
		void onReady(core::ObjectRef parent, core::LevelRef level) override;
		void onCleanup() override;
		void step(const core::time_state &time) override;
		cpBB getBB() const override;

	protected:

		void onBodyWillBeDestroyed(core::PhysicsComponentRef physics, cpBody *body);
		void onShapeWillBeDestroyed(core::PhysicsComponentRef physics, cpShape *shape);

	protected:

		config _config;
		bool _automaticallyAttach;
		cpBody *_sacBody, *_attachedToBody;
		cpShape *_sacShape, *_attachedToShape;
		cpConstraint *_attachmentSpring;
		dvec2 _up, _right;
		double _mass;

	};

#pragma mark - EggsacSpawnComponent

	class EggsacSpawnComponent : public core::Component {
	public:

		struct config {
			// number of times we can spawn a flock
			size_t spawnCount;

			// number of elements in a flock
			size_t spawnSize;

			// config for BoidFlockController which builds and spawns the boid flock
			BoidFlock::config flock;
		};

	public:

		EggsacSpawnComponent(config c);
		virtual ~EggsacSpawnComponent();

		// get number of flocks have been spawned
		int getSpawnCount() const;

		// returns true if we can spawn a new flock
		bool canSpawn() const;

		// spawn a new flock
		void spawn();

		void update(const core::time_state &time) override;

	protected:

		void onFlockDidFinish(const BoidFlockControllerRef &fc);

	protected:

		config _config;
		int _spawnCount;
		BoidFlockWeakRef _flock;

	};


#pragma mark - Eggsac

	class Eggsac : public core::Entity {
	public:

		struct config {
			EggsacPhysicsComponent::config physics;
			EggsacDrawComponent::config draw;
			EggsacSpawnComponent::config spawn;
			core::HealthComponent::config health;
		};

		static EggsacRef create(string name, dvec2 position, core::util::xml::XmlMultiTree node);

	public:

		Eggsac(string name);
		virtual ~Eggsac();

		// Entity
		void onHealthChanged(double oldHealth, double newHealth) override;
		void onDeath() override;

		// Object
		void update(const core::time_state &time) override;
		void onFinishing(core::seconds_t secondsLeft, double amountFinished) override;


	};



}} // namespace surfacer::enemy

#endif /* Eggsac_hpp */
