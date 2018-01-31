//
//  PrecariouslyConstants.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 10/7/17.
//

#include "PrecariouslyConstants.hpp"

namespace precariously {

    namespace ShapeFilters {
        cpShapeFilter TERRAIN = cpShapeFilterNew(CP_NO_GROUP, _TERRAIN, _TERRAIN | _TERRAIN_PROBE | _GRABBABLE | _ANCHOR | _PLAYER | _ENEMY);
        cpShapeFilter TERRAIN_PROBE = cpShapeFilterNew(CP_NO_GROUP, _TERRAIN_PROBE, _TERRAIN);
        cpShapeFilter ANCHOR = cpShapeFilterNew(CP_NO_GROUP, _ANCHOR, _TERRAIN | _PLAYER | _ENEMY);
        cpShapeFilter GRABBABLE = cpShapeFilterNew(CP_NO_GROUP, _GRABBABLE, _TERRAIN | _ENEMY);
        cpShapeFilter PLAYER = cpShapeFilterNew(CP_NO_GROUP, _PLAYER, _TERRAIN | _ANCHOR | _ENEMY);
        cpShapeFilter ENEMY = cpShapeFilterNew(CP_NO_GROUP, _ENEMY, _TERRAIN | _ANCHOR | _PLAYER | _GRABBABLE);
    }

}
