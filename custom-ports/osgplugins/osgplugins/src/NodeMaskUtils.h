#pragma once

#include <osg/Node>

namespace osgRC {
    struct NodeMaskUtils {
		//the mask for all picking visitors:
		static const osg::Node::NodeMask PICKING = 0x00000001; //1 << 0

		//cv.setTraversalMask(NodeMaskUtils::SHADOWS) in rcParallelSplitShadowMap::cull() makes sure only nodes with this bit SET will be rendered into the shadow maps
		static const osg::Node::NodeMask SHADOWS = 0x00000020; //1 << 5

		//main camera and left eye camera gets culled with cullmask = NodeMaskUtils::RENDER_LEFT_EYE  | NodeMaskUtils::COMPABILITY_BIT
		//               right eye camera gets culled with cullmask = NodeMaskUtils::RENDER_RIGHT_EYE | NodeMaskUtils::COMPABILITY_BIT

		//rtt camera for picture in frame (clipplane) gets culled with cullmask = RENDER_IN_RTT_CAMERA

//		static const osg::Node::NodeMask RTT_CAM =   0x00000040; //1 << 6
//		static const osg::Node::NodeMask COMPAT =    0x00000080; //1 << 7

//		static const osg::Node::NodeMask LEFT_EYE =  0x10000000; //1 << 28
//		static const osg::Node::NodeMask RIGHT_EYE = 0x20000000; //1 << 29




        static const osg::Node::NodeMask ENABLE_ALL = 0xffffffff;
        static const osg::Node::NodeMask DISABLE_SHADOWS = 0xffffffdf; //bit 27
        static const osg::Node::NodeMask DISABLE_PICKING = 0xfffffffe; //bit 32
        static const osg::Node::NodeMask DISABLE_ALL = 0x0;
        static const osg::Node::NodeMask COMPABILITY_BIT = 0x00000080; // bit 25 set. Nodemasks of older osg versions are 0xff by default (which is 0x000000ff) .
                                                                               // This prevents us for looking only at one bitwith if we look for bits 1 to 24.
                                                                               // So for nodemasks that are using those bits we set the compability bit to also work with old osg files.
        static const osg::Node::NodeMask RENDER_RIGHT_EYE = 0x10000000; //bit 4
        static const osg::Node::NodeMask RENDER_LEFT_EYE = 0x20000000; //bit 3
        static const osg::Node::NodeMask RENDER_IN_RTT_CAMERA = 0x00000040; //bit 26
        static const osg::Node::NodeMask RENDER_BOTH_EYES = RENDER_RIGHT_EYE|RENDER_LEFT_EYE|RENDER_IN_RTT_CAMERA; // bit 3 and 4

		//Normal active object has on Switch nodemask = RENDER_BOTH_EYES | PICKING | SHADOWS
		//deleted active object gets nodemask 0 on Switch

		//special effects on nodes:
		//invisible but pickable: = 0x00000001
		//visible but NOT pickable: 0xfffffffe
		//only visible for left eye: 0xefffff7f (clear RENDER_RIGHT_EYE and COMPABILITY_BIT)
		//only visible for left eye: 0x20000000 (only RENDER_LEFT_EYE) : not pickable, no shadows, not visible in rtt camera

    };
}