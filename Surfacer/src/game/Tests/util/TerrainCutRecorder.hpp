//
//  TerrainCutRecorder.hpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 3/24/18.
//

#ifndef TerrainCutRecorder_hpp
#define TerrainCutRecorder_hpp

#include "Core.hpp"

class TerrainCutRecorder : public core::signals::receiver {
public:
    
    struct cut {
        dvec2 a;
        dvec2 b;
        double width;
        
        cut(){}
        
        cut(dvec2 a, dvec2 b, double width):
        a(a),b(b),width(width)
        {}
        
        string toString() const {
            stringstream ss;
            ss << "[(" << a.x << "," << a.y << ")|(" << b.x << "," << b.y << ")|" << width << "]";
            return ss.str();
        }
        
        static bool fromString(string desc, cut &c);
    };
    
public:
    
    TerrainCutRecorder(const fs::path &store);
    ~TerrainCutRecorder();
    
    void addCut(dvec2 a, dvec2 b, double width);
    void clearCuts();
    void save() const { persist(); }
    
    const vector<cut> &getCuts() const { return _cuts; }

private:

    void load();
    void persist() const;
    
private:
    
    fs::path _store;
    vector<cut> _cuts;
    
};

#endif /* TerrainCutRecorder_hpp */
