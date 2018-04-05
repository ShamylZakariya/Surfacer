//
//  GlslProgLoader.hpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 4/5/18.
//

#ifndef GlslProgLoader_hpp
#define GlslProgLoader_hpp

#include <cinder/gl/GlslProg.h>

namespace core {
    namespace util {

        ci::gl::GlslProgRef loadGlsl(const ci::DataSourceRef &glslDataSource);

        ci::gl::GlslProgRef loadGlslAsset(const std::string &assetName);
        
    }
}

#endif /* GlslProgLoader_hpp */
