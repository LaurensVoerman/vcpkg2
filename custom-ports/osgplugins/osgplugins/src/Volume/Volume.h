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

#ifndef OSGVOLUME_tile
#define OSGVOLUME_tile 1

#include <osg/Group>
#include <osg/Image>

#include <osgDB/ReaderWriter>
#include "Property.h"

namespace osg_ibr {

    class RayTracedTechnique;

    /** Volume provides a framework for loosely coupling 3d image data with rendering algorithms.
      * This allows TerrainTechnique's to be plugged in at runtime.*/
    class Volume : public osg::Node
    {
        public:

		    enum ShadingModel
		    {
			    Standard = 0,
			    Light = 1,
			    Isosurface = 2,
			    MaximumIntensityProjection = 3,
			    StandardWithGradientMagnitude = 4,
			    StandardWithGradientMagnitudeAndLight = 5,
                StandardNoTransferfunction = 6
		    };

            Volume();

            /** Copy constructor using CopyOp to manage deep vs shallow copy.*/
            Volume(const Volume&,const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY);

            META_Node(osg_ibr, Volume);

            virtual void traverse(osg::NodeVisitor& nv);

            /** Call init on any attached TerrainTechnique.*/
            void init();

		    /** Get the RayTracedTechnique that will be used to render this tile. */
            RayTracedTechnique* getVolumeTechnique() { return _volumeTechnique.get(); }

            /** Get the const RayTracedTechnique that will be used to render this tile. */
            const RayTracedTechnique* getVolumeTechnique() const { return _volumeTechnique.get(); }

            /** Set the dirty flag on/off.*/
            void setDirty(bool dirty);

            /** return true if the tile is dirty and needs to be updated,*/
            bool getDirty() const { return _dirty; }

            /** Specify whether ImageLayer requires update traversal. */
            bool requiresUpdateTraversal() const;

            typedef std::vector<osg::ref_ptr<osg::Image> > VolumeList;
            void setVolumeList(const VolumeList &volumeList);

            /** Return image associated with layer. */
            virtual VolumeList &getVolumeList() { return _volumeList; }

            /** Return const image associated with layer. */
            virtual const VolumeList & getVolumeList() const { return _volumeList; }

            /** Set the Property (or Properties via the CompositeProperty) that informs the RayTracedTechnique how this layer should be rendered.*/
            void setProperty(Property* property) { _property = property; }

            /** Get the Property that informs the RayTracedTechnique how this layer should be rendered.*/
            Property* getProperty() { return _property.get(); }

            /** Get the const Property that informs the RayTracedTechnique how this layer should be rendered.*/
            const Property* getProperty() const { return _property.get(); }

            /** Call update on the Layer.*/
            void update(osg::NodeVisitor& /*nv*/);

            /** increment the modified count."*/
            void dirty();

            virtual osg::BoundingSphere computeBound() const;

            float getFPS() const { return _fps; }
            void setFPS(float v) { _fps = v; }

        protected:

            virtual ~Volume();

            bool                                _dirty;
            osg::ref_ptr<RayTracedTechnique>    _volumeTechnique;

            osg::ref_ptr<Property>       _property;

            VolumeList    _volumeList;
            float _fps;
    };

}

#endif
