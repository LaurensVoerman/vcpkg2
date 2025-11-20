/* -*-c++-*- OpenSceneGraph - Copyright (C) 1998-2009 Robert Osfield
 *
 * This library is open source and may be redistributed and/or modified under
 * the terms of the OpenSceneGraph Public License (OSGPL) version 0.0 or
 * (at your option) any later version.  The full license is in LICENSE file
 * included with this distribution, and on the openscenegraph.org website.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * OpenSceneGraph Public License for more details.
*/

#ifndef OSGVOLUME_RAYTRACEDTECHNIQUE
#define OSGVOLUME_RAYTRACEDTECHNIQUE 1

#include <osg/Object>

#include <osgUtil/UpdateVisitor>
#include <osgUtil/CullVisitor>
#include <osg/Geode>
#include <osg/Image>
#include "GradientTransferFunction.h"

namespace osg_ibr {

	class Volume;

	class RayTracedTechnique : public osg::Object
	{
    public:

        RayTracedTechnique();

        RayTracedTechnique(const RayTracedTechnique&,const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY);

        META_Object(osg_ibr, RayTracedTechnique);

        void init();

        void cull(osgUtil::CullVisitor* nv);

        /** Traverse the terrain subgraph.*/
        virtual void traverse(osg::NodeVisitor& nv);

        void setClipPlane(float alpha, const osg::Plane &plane);

        void CalculateGradient(const osg::Image &input, osg::Image &output);

		osg::Image *getFradientHistogram() const {return _gradientLengthHistogram.get();}
		osg::Image *getFradientColormap() const {return _gradientTransferFunction->getImage();}
		osg_ibr::GradientTransferFunction* getGradientTransferFunction() const {return _gradientTransferFunction.get();}

		void dirty();

    protected:

        virtual ~RayTracedTechnique();

		void setDirty(bool dirty);

        friend class Volume;
        Volume* _volume;
		
		osg::ref_ptr<osg::Geode> _geode;

        struct VolumeData{
            osg::ref_ptr<osg::StateSet> _stateSet;
            osg::ref_ptr<osg::Image>    _gradientImage;
        };

        osg::ref_ptr<osg::StateSet> _whenMovingStateSet;
        osg::ref_ptr<osg::StateSet> _mainStateset;
        std::vector<VolumeData> _volumeDataList;

        typedef std::map<osgUtil::CullVisitor*, osg::Matrix> ModelViewMatrixMap;

        OpenThreads::Mutex _mutex;
        ModelViewMatrixMap _modelViewMatrixMap;


		osg::ref_ptr<osg::Uniform> _clipPlaneAlpha;
		osg::ref_ptr<osg::Uniform> _clipPlanNormal;
		osg::ref_ptr<osg::Uniform> _clipPlanD;
		osg::ref_ptr<osg::Image> _gradientImage;
		osg::ref_ptr<osg::Image> _gradientImageSmooth;
		osg::ref_ptr<osg::Image> _gradientLengthHistogram;
		osg::ref_ptr<osg::Image> _gradientLengthColormap;
		osg::ref_ptr<osg_ibr::GradientTransferFunction> _gradientTransferFunction;

		bool _initializedOnce;

};

}

#endif
