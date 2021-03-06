/*
    This file is a part of SORT(Simple Open Ray Tracing), an open-source cross
    platform physically based renderer.
 
    Copyright (c) 2011-2018 by Cao Jiayin - All rights reserved.
 
    SORT is a free software written for educational purpose. Anyone can distribute
    or modify it under the the terms of the GNU General Public License Version 3 as
    published by the Free Software Foundation. However, there is NO warranty that
    all components are functional in a perfect manner. Without even the implied
    warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    General Public License for more details.
 
    You should have received a copy of the GNU General Public License along with
    this program. If not, see <http://www.gnu.org/licenses/gpl-3.0.html>.
 */

#pragma once

#include "utility/propertyset.h"
#include "spectrum/spectrum.h"
#include <vector>
#include "utility/creator.h"

#include "spectrum/rgbspectrum.h"
#include "math/vector4.h"

class Bsdf;
class MaterialNode;
class TiXmlElement;
class Fresnel;
class MicroFacetDistribution;
class VisTerm;

#define MAT_NODE_TYPE unsigned int
#define MAT_NODE_CONSTANT   0x1
#define MAT_NODE_INPUT      0x2
#define MAT_NODE_BXDF       0x4
#define MAT_NODE_OPERATOR   0x8
#define MAT_NODE_OUTPUT     0x10
#define MAT_NODE_NONE       0x0

typedef Vector4<float> MaterialPropertyValue;

class MaterialNodeProperty
{
public:
	MaterialNodeProperty():node(0){
	}

	// set node property
	virtual void SetNodeProperty( const string& prop );

	// get node property
	virtual MaterialPropertyValue GetPropertyValue( Bsdf* bsdf );
    
    // update bsdf, for layered brdf
    void UpdateBsdf( Bsdf* bsdf , Spectrum weight );

	// sub node if it has value
	MaterialNode*	node;

	// value
	MaterialPropertyValue value;
};

class MaterialNodePropertyString : public MaterialNodeProperty
{
public:
	// set node property
	virtual void SetNodeProperty( const string& prop );

	// get node property
	virtual MaterialPropertyValue GetPropertyValue( Bsdf* bsdf ) { return MaterialPropertyValue(); }

	// color value
	string	str;
};

// base material node
class MaterialNode
{
public:
	MaterialNode(){
		subtree_node_type = MAT_NODE_NONE;
		m_node_valid = true;
		m_post_processed = false;
	}
	virtual ~MaterialNode();

	// update bsdf
	virtual void UpdateBSDF( Bsdf* bsdf , Spectrum weight = 1.0f );

	// parse property or socket
	virtual void ParseProperty( TiXmlElement* element , MaterialNode* node );

	// parse a new node
	virtual MaterialNode* ParseNode( TiXmlElement* element , MaterialNode* node );

	// get property value, this should never be called
	virtual MaterialPropertyValue	GetNodeValue( Bsdf* bsdf ) { return 0.0f; }

	// post process
	virtual void PostProcess();

	// check validation
	virtual bool CheckValidation();

	// get node type
	virtual MAT_NODE_TYPE getNodeType();

protected:
	// node properties
	std::unordered_map< string , MaterialNodeProperty * > m_props;

	// get node property
	MaterialNodeProperty*	getProperty( const string& name );

	// node type of this sub-tree
	MAT_NODE_TYPE subtree_node_type;

	// valid node
	bool m_node_valid;

	// already post processed
	bool m_post_processed;
};

// Mateiral output node
class OutputNode : public MaterialNode
{
public:
	OutputNode();

	// update bsdf
    void UpdateBSDF( Bsdf* bsdf , Spectrum weight = 1.0f ) override;

	// get node type
    MAT_NODE_TYPE getNodeType() override { return MAT_NODE_OUTPUT | MaterialNode::getNodeType(); }

	// check validation
    bool CheckValidation() override;

private:
	MaterialNodeProperty	output;
};
