#pragma once

//
//  Actions.h
//  Surfacer
//
//  Created by Shamyl Zakariya on 10/4/12.
//
//

#include "GameAction.h"

namespace game {

class RichTextBox;

namespace MessageBoxType
{
	enum type
	{	
		DEFAULT,	/** centered at bottom of screen, no class assigned to HTML body */
		MODAL,		/** centered, fills screen, applies "modal" class to HTML body */
		ALERT,		/** positioned top-right of screen, applies "alert" class to HTML body */
		INFO,		/** positioned top-right of screen, applied "info" tag to HTML body */
		TAG			/** tracks a game object's screen position, applies "tag" tag to HTML body */
	};
	
	std::string toString( type );
	type fromString( const std::string & );

	std::string bodyClass( type );
	Vec2r position( type );

}


class MessageBoxAction : public Action
{
	public:
	
		struct init : public Action::init
		{
			/*
				the message to display, in HTML. For example:
				<p>This is a <em>rich</em> and <strong>bold</strong> message to display</p>
			*/
			std::string message;
			
			// if true, the message box will fill the center of the screen, and pause the game until dismissed
			// otherwise it will sit at the bottom-center of the screen, and will go away when stopped
			MessageBoxType::type type;

			// if type == MessageBoxTYpe::TAG, this MessageBox will track the GameObject with identifier 'targetIdentifier'
			std::string targetIdentifier;
			
			init():
				type(MessageBoxType::DEFAULT)
			{}
			
			//JsonInitializable
			virtual void initialize( const ci::JsonTree &v )
			{
				Action::init::initialize(v);
				JSON_READ(v,message);
				JSON_READ(v,targetIdentifier);
				JSON_ENUM_READ(v,MessageBoxType,type);
			}
				
		};

	public:
	
		MessageBoxAction();
		virtual ~MessageBoxAction();

		JSON_INITIALIZABLE_INITIALIZE();

		virtual void initialize( const init & );
		const init &initializer() const { return _initializer; }

		virtual void start( GameLevel * );
		virtual void stop( GameLevel * );

	private:
	
		init _initializer;
		RichTextBox *_textBox;

};

}