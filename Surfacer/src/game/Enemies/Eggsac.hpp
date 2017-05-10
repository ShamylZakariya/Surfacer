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
		dvec2 getAttachmentPosition() { return _attachmentPosition; }
		dvec2 getUp() { return _up; }
		dvec2 getRight() { return _right; }
		double getAngle() { return _angle; }

		// PhysicsComponent
		void onReady(GameObjectRef parent, LevelRef level) override;
		void step(const time_state &time) override;
		vector<cpBody*> getBodies() const override { return _bodies; }
		cpBB getBB() const override;

	protected:


	protected:

		config _config;
		dvec2 _attachmentPosition, _up, _right;
		double _angle;
		vector<cpBody*> _bodies;
		vector<cpShape*> _shapes;
		vector<cpConstraint*> _constraints;

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
