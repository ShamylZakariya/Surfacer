//
//  MarchingSquares.h
//  Surfacer
//
//  Created by Shamyl Zakariya on 11/27/17.
//

#ifndef MarchingSquares_h
#define MarchingSquares_h

#include "Core.hpp"

///////////////////////////////////////////////////////////////////////
// Adapted from "Polygonising A Scalar Field" by Paul Bourke
// http://paulbourke.net/geometry/polygonise/

namespace marching_squares
{
	
	const int EdgeTable[16] = {
		0x0, 	//0000,
		0x9, 	//1001,
		0x3, 	//0011
		0xa, 	//1010
		0x6, 	//0110,
		0xf, 	//1111,
		0x5, 	//0101
		0xc, 	//1100
		0xc, 	//1100
		0x5, 	//0101
		0xf, 	//1111,
		0x6, 	//0110,
		0xa, 	//1010
		0x3, 	//0011
		0x9, 	//1001,
		0x0, 	//0000
	};
	
	// note the sub array is length 5, with a -1 at the end to mark end of array
	const int SegmentTable[16][5] = {
		{-1,-1,-1,-1,-1},
		{0,3,-1,-1,-1},
		{1,0,-1,-1,-1},
		{1,3,-1,-1,-1},
		{2,1,-1,-1,-1},
		{2,1,0,3,-1},
		{2,0,-1,-1,-1},
		{2,3,-1,-1,-1},
		{3,2,-1,-1,-1},
		{0,2,-1,-1,-1},
		{1,0,3,2,-1},
		{1,2,-1,-1,-1},
		{3,1,-1,-1,-1},
		{0,1,-1,-1,-1},
		{3,0,-1,-1,-1},
		{-1,-1,-1,-1,-1}
	};
	
	/**
	 @class segment
	 Segments are submitted to user via segment callback param of march()
	 */
	struct segment {
		dvec2 a,b;
		
		segment(void) {}
		segment(const dvec2 &A, const dvec2 &B):a(A),b(B){}
	};
	
#pragma mark -
#pragma mark MarchingSquares
	
	struct grid_cell {
		dvec2 v[4];
		double val[4];
		
		grid_cell(){}
	};
	
	
	/*
	 Linearly interpolate the position where an isosurface cuts
	 an edge between two voxels, each with their own scalar value
	 */
	
	inline dvec2 VertexInterp( double isolevel, const dvec2 &p1, const dvec2 &p2, double valp1, double valp2 ) {
		const double Epsilon = 1e-4;
		double mu;
		dvec2 p;
		
		
		if ( std::abs(isolevel-valp1) < Epsilon ) {
			return p1;
		}
		
		if ( std::abs(isolevel-valp2) < Epsilon ) {
			return p2;
		}
		
		if ( std::abs(valp1-valp2) < Epsilon ) {
			return p1;
		}
		
		mu = (isolevel - valp1) / (valp2 - valp1);
		p.x = p1.x + mu * (p2.x - p1.x);
		p.y = p1.y + mu * (p2.y - p1.y);
		
		return p;
	}
	
	template< class VOXELSTORE >
	bool GetGridCell( int x, int y, const VOXELSTORE &voxels, grid_cell &cell ) {
		// store the value in the voxel array -- clockwise
		cell.val[0] = voxels.valueAt( x  , y   );
		cell.val[1] = voxels.valueAt( x+1, y   );
		cell.val[2] = voxels.valueAt( x+1, y+1 );
		cell.val[3] = voxels.valueAt( x  , y+1 );
		
		if (cell.val[0] > 0.0 ||
			cell.val[1] > 0.0 ||
			cell.val[2] > 0.0 ||
			cell.val[3] > 0.0 )
		{
			// store the location in the voxel array
			cell.v[0].x = x  ; cell.v[0].y = y;
			cell.v[1].x = x+1; cell.v[1].y = y;
			cell.v[2].x = x+1; cell.v[2].y = y+1;
			cell.v[3].x = x  ; cell.v[3].y = y+1;
			
			return true;
		}
		
		return false;
	}
	
	
	/*
	 Given a grid_cell and isolevel, compute the Segments required to represent
	 the isosurface through the cell.
	 Return the number of segments, the array 'segments' will be populated
	 with up to 2 Segments. Returns the number of Segments computed.
	 
	 */
	
	inline int Polygonise( const grid_cell &grid, double isolevel, segment *segments ) {
		//
		//	Determine the index into the edge table which
		//	tells us which voxels are inside of the surface
		//
		
		int squareIndex = 0;
		if (grid.val[0] < isolevel) { squareIndex |= 1; }
		if (grid.val[1] < isolevel) { squareIndex |= 2; }
		if (grid.val[2] < isolevel) { squareIndex |= 4; }
		if (grid.val[3] < isolevel) { squareIndex |= 8; }
		
		//
		//	Square is entirely in/out of the surface
		//
		
		if (EdgeTable[squareIndex] == 0) {
			return 0;
		}
		
		//
		//	Find the voxels where the surface intersects the cube
		//
		
		dvec2 vertlist[4];
		
		if (EdgeTable[squareIndex] & 1) {
			vertlist[0] = VertexInterp(isolevel,grid.v[0],grid.v[1],grid.val[0],grid.val[1]);
		}
		
		if (EdgeTable[squareIndex] & 2) {
			vertlist[1] = VertexInterp(isolevel,grid.v[1],grid.v[2],grid.val[1],grid.val[2]);
		}
		
		if (EdgeTable[squareIndex] & 4) {
			vertlist[2] = VertexInterp(isolevel,grid.v[2],grid.v[3],grid.val[2],grid.val[3]);
		}
		
		if (EdgeTable[squareIndex] & 8) {
			vertlist[3] = VertexInterp(isolevel,grid.v[3],grid.v[0],grid.val[3],grid.val[0]);
		}
		
		//
		//	Finally, generate line segments
		//
		
		int nSegments = 0;
		for ( int i = 0; SegmentTable[squareIndex][i] != -1; i += 2 ) {
			segments[nSegments].a = vertlist[SegmentTable[squareIndex][i  ]];
			segments[nSegments].b = vertlist[SegmentTable[squareIndex][i+1]];
			nSegments++;
		}
		
		return nSegments;
	}
	
#pragma mark -
#pragma mark Public API
	
	
	/**
	 March a 2D voxel space, invoking the segment callback on each generated segment
	 
	 VOXELSPACE is an object with the following interface:
	 
	 - ivec2 min() const;
	 - ivec2 max() const;
	 - double valueAt( int x, int y ) const;
	 
	 SEGCALLBACK is an object with the following interface:
	 
	 - void operator()( int x, int y, const marching_squares::segment &seg )
	 
	 
	 */
	
	template< class VOXELSTORE, class SEGCALLBACK >
	void march( const VOXELSTORE &voxels, SEGCALLBACK &sc, double isolevel = 0.5 ) {
		segment segments[2];
		grid_cell cell;
		ivec2 min = voxels.min(), max = voxels.max();
		
		for ( int y = min.y-1; y <= max.y; y++ ) {
			for ( int x = min.x-1; x <= max.x; x++ ) {
				if ( GetGridCell( x,y, voxels, cell ) ) {
					for ( int s = 0, nSegments = Polygonise( cell, isolevel, segments ); s < nSegments; s++ ) {
						sc( x,y, segments[s] );
					}
				}
			}
		}
	}
	
}

#endif /* MarchingSquares_h */
