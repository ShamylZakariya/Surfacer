//
//  BlendMode.hpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 4/4/17.
//
//

#ifndef BlendMode_h
#define BlendMode_h

#include <cinder/gl/gl.h>

#define ALPHA_EPSILON (1.0/255.0)

namespace core {

	class BlendMode
	{
	public:

		BlendMode():
		_srcColor( GL_SRC_ALPHA ),
		_dstColor( GL_ONE_MINUS_SRC_ALPHA ),
		_srcAlpha( GL_ONE ),
		_dstAlpha( GL_ONE ),
		_blendEqColor( GL_FUNC_ADD ),
		_blendEqAlpha( GL_FUNC_ADD )
		{}

		BlendMode( const BlendMode &c ):
		_srcColor(c._srcColor),
		_dstColor(c._dstColor),
		_srcAlpha( c._srcAlpha ),
		_dstAlpha( c._dstAlpha ),
		_blendEqColor( c._blendEqColor ),
		_blendEqAlpha( c._blendEqAlpha )
		{}

		BlendMode( GLenum src, GLenum dst,
				  GLenum srcAlpha = GL_ONE, GLenum dstAlpha = GL_ONE,
				  GLenum blendEqColor = GL_FUNC_ADD, GLenum blendEqAlpha = GL_FUNC_ADD ):
		_srcColor(src),
		_dstColor(dst),
		_srcAlpha( srcAlpha ),
		_dstAlpha( dstAlpha ),
		_blendEqColor( blendEqColor ),
		_blendEqAlpha( blendEqAlpha )
		{}

		GLenum src() const { return _srcColor; }
		GLenum dst() const { return _dstColor; }
		GLenum srcAlpha() const { return _srcAlpha; }
		GLenum dstAlpha() const { return _dstAlpha; }
		GLenum blendEquation() const { return _blendEqColor; }
		GLenum blendEquationAlpha() const { return _blendEqAlpha; }


		bool operator == ( const BlendMode &other ) const
		{
			return
			_srcColor == other._srcColor &&
			_dstColor == other._dstColor &&
			_srcAlpha == other._srcAlpha &&
			_dstAlpha == other._dstAlpha &&
			_blendEqColor == other._blendEqColor &&
			_blendEqAlpha == other._blendEqAlpha;
		}

		bool operator != ( const BlendMode &other ) const
		{
			return
			_srcColor != other._srcColor ||
			_dstColor != other._dstColor ||
			_srcAlpha != other._srcAlpha ||
			_dstAlpha != other._dstAlpha ||
			_blendEqColor != other._blendEqColor ||
			_blendEqAlpha != other._blendEqAlpha;

		}

		BlendMode &operator = ( const BlendMode &rhs )
		{
			_srcColor = rhs._srcColor;
			_dstColor = rhs._dstColor;
			_srcAlpha = rhs._srcAlpha;
			_dstAlpha = rhs._dstAlpha;
			_blendEqColor = rhs._blendEqColor;
			_blendEqAlpha = rhs._blendEqAlpha;
			return *this;
		}

		void bind() const
		{
			glEnable( GL_BLEND );
			glBlendFuncSeparate(_srcColor, _dstColor, _srcAlpha, _dstAlpha );
			glBlendEquationSeparate( _blendEqColor, _blendEqAlpha );
		}

	private:

		GLenum _srcColor, _dstColor, _srcAlpha, _dstAlpha, _blendEqColor, _blendEqAlpha;

	};

}

std::ostream &operator << ( std::ostream &os, const core::BlendMode &blend );

#endif /* BlendMode_h */
