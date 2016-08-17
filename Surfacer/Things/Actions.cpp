//
//  Actions.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 10/4/12.
//
//

#include "Actions.h"
#include "UIStack.h"
#include "PlayerHud.h"
#include "GameLevel.h"
#include "GameScenario.h"
#include "StringLib.h"

#include <cinder/app/App.h>

using namespace ci;
namespace game {

#pragma mark - MessageBoxType

namespace MessageBoxType
{
	std::string toString( type t )
	{
		switch( t )
		{
			case DEFAULT: return "DEFAULT"; break;
			case MODAL: return "MODAL"; break;
			case ALERT: return "ALERT"; break;
			case INFO: return "INFO"; break;
			case TAG: return "TAG"; break;
		}
		
		return "[UNKNOWN MessageBoxType]";
	}
	
	type fromString( const std::string &t )
	{
		std::string uct = core::util::stringlib::uppercase(t);
		if ( uct == "MODAL" ) return MODAL;
		if ( uct == "ALERT" ) return ALERT;
		if ( uct == "INFO" ) return INFO;
		if ( uct == "TAG" ) return TAG;
		return DEFAULT;
	}

	std::string bodyClass( type t )
	{
		switch( t )
		{
			case DEFAULT: return ""; break;
			case MODAL: return "modal"; break;
			case ALERT: return "alert"; break;
			case INFO: return "info"; break;
			case TAG: return "tag"; break;
		}
		
		return "";
	}

	Vec2r position( type t )
	{
		switch( t )
		{
			case DEFAULT: return Vec2r(0.5,0); break;
			case MODAL: return Vec2r(0.5,0.5); break;
			case ALERT: return Vec2r(1,1); break;
			case INFO: return Vec2r(1,1); break;
			case TAG: return Vec2r(0,0); break; // ignored by RichTextBox
		}

		return Vec2r(0,0);
	}

}

#pragma mark - MessageBoxAction

/*
		init _initializer;
		RichTextBox *_textBox;
*/

CLASSLOAD(MessageBoxAction);
MessageBoxAction::MessageBoxAction():
	_textBox(NULL)
{}

MessageBoxAction::~MessageBoxAction()
{
	if ( _textBox ) _textBox->fadeOut();
}

void MessageBoxAction::initialize( const init &i )
{
	Action::initialize(i);
	_initializer = i;
}

void MessageBoxAction::start( GameLevel *level )
{
	if ( _textBox ) _textBox->fadeOut();

	//
	//	If we're sending out a message for an object which doesn't exist, skip it
	//

	if ( !_initializer.targetIdentifier.empty() )
	{
		if ( !level->objectById(_initializer.targetIdentifier ) )
		{
			return;
		}
	}

	core::ui::Stack *stack = level->scenario()->uiStack();

	game::RichTextBox::init rtbInit;
	rtbInit.templateHtmlFile = "MessageBox";
	rtbInit.htmlContent = _initializer.message;
	rtbInit.bodyClass = MessageBoxType::bodyClass( _initializer.type );
	rtbInit.modal = _initializer.type == MessageBoxType::MODAL;
	rtbInit.position = MessageBoxType::position( _initializer.type );
	rtbInit.trackingTargetIdentifier = _initializer.targetIdentifier;
	rtbInit.displayTime = 0; // no timeout
	rtbInit.minimumDisplayTime = 2; // show for at least two seconds

	game::RichTextBox *textBox = new game::RichTextBox();
	textBox->initialize( rtbInit );
	textBox->fadeIn();
	
	level->notificationLayer()->addView( textBox );
	
	//
	//	A non-modal text box should go away in stop() - but Modal will be dismissed by user interaction
	//

	if ( _initializer.type != MessageBoxType::MODAL )
	{
		_textBox = textBox;
	}
	
}

void MessageBoxAction::stop( GameLevel *level )
{
	if ( _textBox ) _textBox->fadeOut();
}

}