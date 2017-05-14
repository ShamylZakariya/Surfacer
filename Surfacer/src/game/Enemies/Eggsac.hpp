//
//  Eggsac.hpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 5/8/17.
//
//

#ifndef Eggsac_hpp
#define Eggsac_hpp

#include "Core.hpp"
#include "Terrain.hpp"

#include <cinder/Xml.h>

namespace core { namespace game { namespace enemy {

	SMART_PTR(Eggsac);
	SMART_PTR(EggsacPhysicsComponent);
	SMART_PTR(EggsacDrawComponent);

	class EggsacDrawComponent : public DrawComponent {
	public:

		struct config {
		};

	public:

		EggsacDrawComponent(config c);
		virtual ~EggsacDrawComponent();

		config getConfig() const { return _config; }

		// DrawComponent
		void onReady(GameObjectRef parent, LevelRef level) override;

		void draw(const render_state &renderState) override;
		VisibilityDetermination::style getVisibilityDetermination() const override { return VisibilityDetermination::FRUSTUM_CULLING; }
		int getLayer() const override;
		int getDrawPasses() const override { return 1; }
		BatchDrawDelegateRef getBatchDrawDelegate() const override { return nullptr; }

	protected:

		config _config;
		EggsacPhysicsComponentWeakRef _physics;

	};

	class EggsacPhysicsComponent : public PhysicsComponent {
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
		dvec2 getPosition() { return v2(cpBodyGetPosition(_sacBody)); }
		dvec2 getUp() { return _up; }
		dvec2 getRight() { return _right; }
		double getAngle() { return _angle; }

		cpBody*	getBody() const { return _sacBody; }
		cpShape* getBodyShape() const { return _sacShape; }
		cpConstraint *getAttachmentSpring() const { return _attachmentSpring; }
		cpConstraint *getOrientationSpring() const { return _orientationSpring; }

		bool isAttached() const;

		// PhysicsComponent
		void onReady(GameObjectRef parent, LevelRef level) override;
		void onCleanup() override;
		void step(const time_state &time) override;
		cpBB getBB() const override;

	protected:

		void onBodyWillBeDestroyed(PhysicsComponentRef physics, cpBody *body);
		void onShapeWillBeDestroyed(PhysicsComponentRef physics, cpShape *shape);
		
		void attach();
		void detach();

	protected:

		config _config;
		cpBody *_sacBody, *_attachedToBody;
		cpShape *_sacShape, *_attachedToShape;
		cpConstraint *_attachmentSpring, *_orientationSpring;
		dvec2 _up, _right;
		double _angle, _mass;

	};


#pragma mark - Eggsac

	class Eggsac : public GameObject {
	public:

		struct config {
			EggsacPhysicsComponent::config physics;
			EggsacDrawComponent::config draw;
		};

		static EggsacRef create(string name, dvec2 position, ci::XmlTree node);

	public:

		Eggsac(string name);
		virtual ~Eggsac();

		// GameObject
		void update(const time_state &time) override;


	};



}}} // namespace core::game::enemy

#endif /* Eggsac_hpp */
