#pragma once

/*
 *  GraphTraversal.h
 *  CuttingExperiments
 *
 *  Created by Shamyl Zakariya on 12/22/10.
 *  Copyright 2010 Shamyl Zakariya. All rights reserved.
 *
 */

#include "Terrain.h"

#include <list>
#include <set>
#include <queue>

namespace game { namespace terrain { namespace floodfill {

namespace {

	std::size_t _visit_tag = 1;

}
	
/**
	Visit every node reachable from origin, calling Visitor::operator(Voxel*) on that node until all nodes are reached, or Visitor::operator(Voxel*) returns false.

	Return true if all reachable nodes were reached without the visitor aborting search
*/

template< class V, class T >
bool visit( Voxel *origin, V &visitor, T &test )
{
	/*
		Implementation from Wikipedia's FloodFill page:
		http://en.wikipedia.org/wiki/Flood_fill		

		Flood-fill (node, target-color, replacement-color):
		 1. Set Q to the empty queue.
		 2. If the color of node is not equal to target-color, return.
		 3. Add node to Q.
		 4. For each element n of Q:
		 5.  If the color of n is equal to target-color:
		 6.   Set w and e equal to n.
		 7.   Move w to the west until the color of the node to the west of w no longer matches target-color.
		 8.   Move e to the east until the color of the node to the east of e no longer matches target-color.
		 9.   Set the color of nodes between w and e to replacement-color.
		10.   For each node n between w and e:
		11.    If the color of the node to the north of n is target-color, add that node to Q.
			   If the color of the node to the south of n is target-color, add that node to Q.
		12. Continue looping until Q is exhausted.
		13. Return.		

		Local Implementation Notes:
			- I'm assigning a common tag on visited Vertices to mark they've been touched. It's faster than adding them to a set<>
	*/
	
	std::queue<Voxel*> Q;
	
	if ( !test(origin)) return true;
	Q.push( origin );
	
	// we're using a global monotonically increasing tag value
	std::size_t tag = ++_visit_tag;
		
	while( !Q.empty() )
	{
		Voxel *n = Q.front();
		Q.pop();
		
		if ( test(n) && (n->tag != tag))
		{
			Voxel *w = n;
					  
			while( test(w->neighbors[Compass::West]))
			{
				w = w->neighbors[Compass::West];
			}

			// iterate from westernmost reachable to easternmost reachable
			for ( Voxel *i = w; test(i); i = i->neighbors[Compass::East] )
			{
				if ( i->tag != tag )
				{
					i->tag = tag;
					if ( !visitor( i )) return false;
										
					Voxel *nw = i->neighbors[Compass::NorthWest],
						   *n = i->neighbors[Compass::North],
						   *ne = i->neighbors[Compass::NorthEast],
						   *sw = i->neighbors[Compass::SouthWest],
						   *s = i->neighbors[Compass::South],
						   *se = i->neighbors[Compass::SouthEast];
							  
					if ( test(nw) && (nw->tag != tag)) Q.push( nw );
					if ( test(n ) && (n->tag  != tag)) Q.push( n  );
					if ( test(ne) && (ne->tag != tag)) Q.push( ne );

					if ( test(sw) && (sw->tag != tag)) Q.push( sw );
					if ( test(s ) && (s->tag  != tag)) Q.push( s  );
					if ( test(se) && (se->tag != tag)) Q.push( se );
				}
			}
		}		
	}
	
	return true;
}


}}} // end namespace game::terrain::floodfill