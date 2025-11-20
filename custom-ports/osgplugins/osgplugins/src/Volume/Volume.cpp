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

#include "Volume.h"
#include "Property.h"
#include "RayTracedTechnique.h"

#include <osgDB/ReadFile>
#include <osgDB/Registry>
#include <osgDB/Input>
#include <osgDB/Output>
#include <osgDB/ParameterOutput>
#include <osgDB/ObjectWrapper>
#include <osgDB/InputStream>
#include <osgDB/OutputStream>
#include <osg/ImageStream>
#include <osg/Version>


using namespace osg;
using namespace osg_ibr;

/////////////////////////////////////////////////////////////////////////////////
//
// Volume
//
Volume::Volume():
    _dirty(false),
	_volumeTechnique(new RayTracedTechnique()),
    _fps(0)
{
	_volumeTechnique->_volume = this;
	setDirty(true);

    setNumChildrenRequiringUpdateTraversal(1);
}

Volume::Volume(const Volume& volume, const osg::CopyOp& copyop):
Node(volume, copyop),
    _dirty(false),
    _volumeTechnique(volume._volumeTechnique),
    _property(volume._property),
    _volumeList(volume._volumeList),
    _fps(0)
{
    setNumChildrenRequiringUpdateTraversal(getNumChildrenRequiringUpdateTraversal()+1);
	setDirty(true);
}

Volume::~Volume()
{
}

void Volume::traverse(osg::NodeVisitor& nv)
{
    if (nv.getVisitorType()==osg::NodeVisitor::UPDATE_VISITOR &&
        requiresUpdateTraversal())
    {
        update(nv);
    }

    if (_volumeTechnique.valid())
    {
        _volumeTechnique->traverse(nv);
    }
    else
    {
        osg::Node::traverse(nv);
    }
}

bool Volume::requiresUpdateTraversal() const {
    return _volumeList.size() > 1;
}

void Volume::setVolumeList(const VolumeList &volumeList) {
    _volumeList = volumeList;
}

void Volume::dirty() {
    for(VolumeList::iterator it = _volumeList.begin(); it != _volumeList.end(); it++){
        it->get()->dirty();
    }
}

void Volume::update(osg::NodeVisitor& nv) {
    for(VolumeList::iterator it = _volumeList.begin(); it != _volumeList.end(); it++){
        it->get()->update(&nv);
    }
}

void Volume::init()
{
    if (_volumeTechnique.valid() && getDirty())
    {
        _volumeTechnique->init();

        setDirty(false);
    }
}

void Volume::setDirty(bool dirty)
{
    if (_dirty==dirty) return;

    _dirty = dirty;

    if (_dirty)
    {
        setNumChildrenRequiringUpdateTraversal(getNumChildrenRequiringUpdateTraversal()+1);
    }
    else if (getNumChildrenRequiringUpdateTraversal()>0)
    {
        setNumChildrenRequiringUpdateTraversal(getNumChildrenRequiringUpdateTraversal()-1);
    }
}

osg::BoundingSphere Volume::computeBound() const
{
	if (_volumeList.size() > 0)
    {
        return osg::BoundingSphere( osg::Vec3(0,0,0), 1.4);
    }
    else
    {
        return osg::BoundingSphere();
    }
}
#if	OSG_MIN_VERSION_REQUIRED(3,3,2)
#else
static bool checkVolumeList(const Volume& image)
{
	return false;
}
static bool readVolumeList(osgDB::InputStream& is, Volume& image)
{
	unsigned int images = is.readSize(); is >> is.BEGIN_BRACKET;
	Volume::VolumeList& imageDataList = image.getVolumeList();
	for (unsigned int i = 0; i<images; ++i)
	{
		osg::Image* img =is.readImage();
		if (img) imageDataList.push_back(img);
	}
	is >> is.END_BRACKET;
	return true;
}

static bool writeVolumeList(osgDB::OutputStream& os, const Volume& image)
{
	const Volume::VolumeList& imageDataList = image.getVolumeList();
	os.writeSize(imageDataList.size()); os << os.BEGIN_BRACKET << std::endl;
	for (Volume::VolumeList::const_iterator itr = imageDataList.begin();
		itr != imageDataList.end();
		++itr)
	{
		os.writeImage((*itr).get());
	}
	os << os.END_BRACKET << std::endl;
	return true;
}
#endif
// osgb/t writer
REGISTER_OBJECT_WRAPPER( osg_ibr_Volume,
                         new osg_ibr::Volume,
                         osg_ibr::Volume,
                         "osg::Object osg::Node osg_ibr::Volume" )
{
    ADD_BOOL_SERIALIZER( Dirty, false );  // _dirty
    ADD_OBJECT_SERIALIZER(Property, osg_ibr::Property, NULL);  // _property
    ADD_FLOAT_SERIALIZER(FPS, 0.0f); // _fps
#if	OSG_MIN_VERSION_REQUIRED(3,3,2)
    ADD_VECTOR_SERIALIZER(VolumeList, osg_ibr::Volume::VolumeList, osgDB::BaseSerializer::RW_IMAGE, 0);
#else
	ADD_USER_SERIALIZER(VolumeList);
#endif
}
