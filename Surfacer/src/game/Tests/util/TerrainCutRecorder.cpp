//
//  TerrainCutRecorder.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 3/24/18.
//

#include "TerrainCutRecorder.hpp"

using namespace core;

namespace {
    
    bool parse_dvec2(string desc, dvec2 &v) {
        if (desc[0] == '(' && desc[desc.size()-1] == ')') {
            desc = desc.substr(1, desc.size()-2);
            auto components = strings::split(desc, ",");
            if (components.size() == 2) {
                try {
                    v.x = stod(components[0]);
                    v.y = stod(components[1]);
                    return true;
                } catch (invalid_argument e) {
                    return false;
                }
            }
        }
        return false;
    }
    
}

bool TerrainCutRecorder::cut::fromString(string desc, cut &c) {
    if (desc[0] == '[' && desc[desc.size()-1] == ']') {
        desc = desc.substr(1, desc.size()-2);

        auto tokens = strings::split(desc, "|");
        if (tokens.size() == 3) {
            if (parse_dvec2(tokens[0], c.a) && parse_dvec2(tokens[1], c.b)) {
                try {
                    c.width = stod(tokens[2]);
                    return true;
                } catch (invalid_argument e) {
                    return false;
                }
            }
        }
    }
    
    return false;
}

/*
 string _store;
 vector<cut> _cuts;
*/
TerrainCutRecorder::TerrainCutRecorder(const fs::path &store):
_store(store)
{
    CI_LOG_D("TerrainCutRecorder store: \"" << store.string() << "\"");
    load();
}

TerrainCutRecorder::~TerrainCutRecorder() {
    persist();
}

void TerrainCutRecorder::addCut(dvec2 a, dvec2 b, double width) {
    _cuts.emplace_back(cut(a,b,width));
}

void TerrainCutRecorder::clearCuts() {
    _cuts.clear();
}

void TerrainCutRecorder::load() {
    _cuts.clear();
    
    if (fs::exists(_store)) {
        auto lines = strings::readFileIntoLines(_store.string());
        for (auto line : lines) {
            cut c;
            if (cut::fromString(line, c)) {
                _cuts.push_back(c);
            }
        }
    }
}

void TerrainCutRecorder::persist() const {
    fstream out(_store.string(), ios::out | ios::trunc);
    
    for (auto c : _cuts) {
        auto line = c.toString() + "\n";
        if (!out.write(line.c_str(), line.length())) {
            CI_LOG_E("Unable to write line to file");
            out.close();
            return;
        }
    }
    
    out.close();
}



