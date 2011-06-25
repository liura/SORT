/*
 * filename :	kdtree.cpp
 *
 * programmer :	Cao Jiayin
 */

// include the header file
#include "kdtree.h"

// default constructor
KDTree::KDTree()
{
}

// constructor from a primitive list
KDTree::KDTree( vector<Primitive*>* l ) : Accelerator(l)
{
}
// destructor
KDTree::~KDTree()
{
}

// get the intersection between the ray and the primitive set
bool KDTree::GetIntersect( const Ray& r , Intersection* intersect ) const
{
	return true;
}

// build the acceleration structure
void KDTree::Build()
{
}