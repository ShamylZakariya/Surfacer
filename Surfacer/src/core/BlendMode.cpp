//
//  BlendMode.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 4/4/17.
//
//

#include "BlendMode.hpp"
#include "Strings.hpp"

namespace {

    std::string blendFactorName(GLenum f) {
        switch (f) {
            case GL_DST_ALPHA:
                return "GL_DST_ALPHA";
            case GL_DST_COLOR:
                return "GL_DST_COLOR";
            case GL_ONE:
                return "GL_ONE";
            case GL_ONE_MINUS_DST_ALPHA:
                return "GL_ONE_MINUS_DST_ALPHA";
            case GL_ONE_MINUS_DST_COLOR:
                return "GL_ONE_MINUS_DST_COLOR";
            case GL_ONE_MINUS_SRC_ALPHA:
                return "GL_ONE_MINUS_SRC_ALPHA";
            case GL_ONE_MINUS_SRC_COLOR:
                return "GL_ONE_MINUS_SRC_COLOR";
            case GL_SRC_ALPHA:
                return "GL_SRC_ALPHA";
            case GL_SRC_ALPHA_SATURATE:
                return "GL_SRC_ALPHA_SATURATE";
            case GL_SRC_COLOR:
                return "GL_SRC_COLOR";
            case GL_ZERO:
                return "GL_ZERO";

        }

        return "[" + core::str(f) + " is not a blend factor]";
    }
}

std::ostream &operator<<(std::ostream &os, const core::BlendMode &blend) {
    return os << "[BlendMode src: " + blendFactorName(blend.src()) + " : " + blendFactorName(blend.dst()) + "]";
}
