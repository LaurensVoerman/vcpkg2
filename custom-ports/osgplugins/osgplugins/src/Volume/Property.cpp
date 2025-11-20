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

#include "Property.h"
#include "Volume.h"

#include <osg/Version>
#include <osgDB/ReadFile>
#include <osgDB/Registry>
#include <osgDB/Input>
#include <osgDB/Output>
#include <osgDB/ParameterOutput>
#include <osgDB/ObjectWrapper>
#include <osgDB/InputStream>
#include <osgDB/OutputStream>

using namespace osg_ibr;

Property::Property()
{
}

Property::Property(const Property& property,const osg::CopyOp& copyop):
    osg::Object(property,copyop)
{
}

Property::~Property()
{
}

/////////////////////////////////////////////////////////////////////////////
//
// CompositeProperty
//
CompositeProperty::CompositeProperty()
{
}

CompositeProperty::CompositeProperty(const CompositeProperty& compositeProperty, const osg::CopyOp& copyop):
    Property(compositeProperty,copyop)
{
}


void CompositeProperty::clear()
{
    _properties.clear();
}

/////////////////////////////////////////////////////////////////////////////
//
// SwitchProperty
//
SwitchProperty::SwitchProperty()
{
}

SwitchProperty::SwitchProperty(const SwitchProperty& switchProperty, const osg::CopyOp& copyop):
    CompositeProperty(switchProperty,copyop),
    _activeProperty(switchProperty._activeProperty)
{
}


/////////////////////////////////////////////////////////////////////////////
//
// TransferFunctionProperty
//
TransferFunctionProperty::TransferFunctionProperty(osg::TransferFunction* tf):
    _tf(tf)
{
}

TransferFunctionProperty::TransferFunctionProperty(const TransferFunctionProperty& tfp, const osg::CopyOp& copyop):
    Property(tfp,copyop),
    _tf(tfp._tf)
{
}

/////////////////////////////////////////////////////////////////////////////
//
// TransferFunctionGradientProperty
//
TransferFunctionGradientProperty::TransferFunctionGradientProperty(osg_ibr::GradientTransferFunction* tf):
    _tf(tf)
{
}

TransferFunctionGradientProperty::TransferFunctionGradientProperty(const TransferFunctionGradientProperty& tfp, const osg::CopyOp& copyop):
    Property(tfp,copyop),
    _tf(tfp._tf)
{
}

/////////////////////////////////////////////////////////////////////////////
//
// TransferFunctionGradientLightingProperty
//
TransferFunctionGradientLightingProperty::TransferFunctionGradientLightingProperty(osg_ibr::GradientTransferFunction* tf):
    _tf(tf)
{
}

TransferFunctionGradientLightingProperty::TransferFunctionGradientLightingProperty(const TransferFunctionGradientLightingProperty& tfp, const osg::CopyOp& copyop):
    Property(tfp,copyop),
    _tf(tfp._tf)
{
}

/////////////////////////////////////////////////////////////////////////////
//
// ScalarProperty
//
ScalarProperty::ScalarProperty()
{
    _uniform = new osg::Uniform;
}

ScalarProperty::ScalarProperty(const std::string& scalarName, float value)
{
    setName(scalarName);
    _uniform = new osg::Uniform(scalarName.c_str(), value);
}

ScalarProperty::ScalarProperty(const ScalarProperty& sp, const osg::CopyOp& copyop):
    Property(sp,copyop)
{
    _uniform = new osg::Uniform(sp._uniform->getName().c_str(), getValue());

}

/////////////////////////////////////////////////////////////////////////////
//
// IsoSurfaceProperty
//
IsoSurfaceProperty::IsoSurfaceProperty(float value):
    ScalarProperty("IsoSurfaceValue",value)
{
}

IsoSurfaceProperty::IsoSurfaceProperty(const IsoSurfaceProperty& isp,const osg::CopyOp& copyop):
    ScalarProperty(isp, copyop)
{
}


/////////////////////////////////////////////////////////////////////////////
//
// MaximumIntensityProjectionProperty
//
MaximumIntensityProjectionProperty::MaximumIntensityProjectionProperty()
{
}

MaximumIntensityProjectionProperty::MaximumIntensityProjectionProperty(const MaximumIntensityProjectionProperty& isp,const osg::CopyOp& copyop):
    Property(isp, copyop)
{
}

/////////////////////////////////////////////////////////////////////////////
//
// LightingProperty
//
LightingProperty::LightingProperty()
{
}

LightingProperty::LightingProperty(const LightingProperty& isp,const osg::CopyOp& copyop):
    Property(isp, copyop)
{
}


/////////////////////////////////////////////////////////////////////////////
//
// SampleDensityProperty
//
SampleDensityProperty::SampleDensityProperty(float value):
    ScalarProperty("SampleDensityValue",value)
{
}

SampleDensityProperty::SampleDensityProperty(const SampleDensityProperty& isp,const osg::CopyOp& copyop):
    ScalarProperty(isp, copyop)
{
}

/////////////////////////////////////////////////////////////////////////////
//
// SampleDensityJitterProperty
//
SampleDensityJitterProperty::SampleDensityJitterProperty(float value):
ScalarProperty("SampleDensityJitterValue", value) {}

SampleDensityJitterProperty::SampleDensityJitterProperty(const SampleDensityJitterProperty& isp, const osg::CopyOp& copyop) :
ScalarProperty(isp, copyop) {}


/////////////////////////////////////////////////////////////////////////////
//
// SampleDensityWhenMovingProperty
//
SampleDensityWhenMovingProperty::SampleDensityWhenMovingProperty(float value):
    ScalarProperty("SampleDensityWhenMovingValue",value)
{
}

SampleDensityWhenMovingProperty::SampleDensityWhenMovingProperty(const SampleDensityWhenMovingProperty& isp,const osg::CopyOp& copyop):
    ScalarProperty(isp, copyop)
{
}


/////////////////////////////////////////////////////////////////////////////
//
// PropertyVisitor
//
PropertyVisitor::PropertyVisitor(bool traverseOnlyActiveChildren):
    _traverseOnlyActiveChildren(traverseOnlyActiveChildren)
{
}

void PropertyVisitor::apply(CompositeProperty& cp)
{
    for(unsigned int i=0; i<cp.getNumProperties(); ++i)
    {
        cp.getProperty(i)->accept(*this);
    }
}

void PropertyVisitor::apply(SwitchProperty& sp)
{
    if (_traverseOnlyActiveChildren)
    {
        if (sp.getActiveProperty()>=0 && sp.getActiveProperty()<static_cast<int>(sp.getNumProperties()))
        {
            sp.getProperty(sp.getActiveProperty())->accept(*this);
        }
    }
    else
    {
        for(unsigned int i=0; i<sp.getNumProperties(); ++i)
        {
            sp.getProperty(i)->accept(*this);
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
//
// CollectPropertiesVisitor
//
CollectPropertiesVisitor::CollectPropertiesVisitor(bool traverseOnlyActiveChildren):
    PropertyVisitor(traverseOnlyActiveChildren)
{
}

void CollectPropertiesVisitor::apply(Property&) {}
void CollectPropertiesVisitor::apply(TransferFunctionProperty& tf) { _tfProperty = &tf; }
void CollectPropertiesVisitor::apply(ScalarProperty&) {}
void CollectPropertiesVisitor::apply(IsoSurfaceProperty& iso) { _isoProperty = &iso; }
void CollectPropertiesVisitor::apply(MaximumIntensityProjectionProperty& mip) { _mipProperty = &mip; }
void CollectPropertiesVisitor::apply(LightingProperty& lp) { _lightingProperty = &lp; }
void CollectPropertiesVisitor::apply(TransferFunctionGradientProperty& lp) { _transferFunctionGradientProperty = &lp; }
void CollectPropertiesVisitor::apply(TransferFunctionGradientLightingProperty& lp) { _transferFunctionGradientLightingProperty = &lp; }
void CollectPropertiesVisitor::apply(SampleDensityProperty& sdp) { _sampleDensityProperty = &sdp; }
void CollectPropertiesVisitor::apply(SampleDensityWhenMovingProperty& sdp) { _sampleDensityWhenMovingProperty = &sdp; }
void CollectPropertiesVisitor::apply(SampleDensityJitterProperty& sdp) { _sampleDensityJitterProperty = &sdp; }

static bool checkProperties( const CompositeProperty& prop )
{
    return prop.getNumProperties()>0;
}

static bool readProperties( osgDB::InputStream& is, CompositeProperty& prop )
{
    unsigned int size = 0; is >> size >> is.BEGIN_BRACKET;
    for ( unsigned int i=0; i<size; ++i )
    {
		osg::ref_ptr<Property> child = is.readObjectOfType<Property>();
		if (child.valid()) prop.addProperty(child.get());
    }
    is >> is.END_BRACKET;
    return true;
}

static bool writeProperties( osgDB::OutputStream& os, const CompositeProperty& prop )
{
    unsigned int size = prop.getNumProperties();
    os << size << os.BEGIN_BRACKET << std::endl;
    for ( unsigned int i=0; i<size; ++i )
    {
        os << prop.getProperty(i);
    }
    os << os.END_BRACKET << std::endl;
    return true;
}

namespace osg_ibr_CompositeProperty
{
	REGISTER_OBJECT_WRAPPER( osg_ibr_CompositeProperty,
                            new osg_ibr::CompositeProperty,
                            osg_ibr::CompositeProperty,
							 "osg::Object osg_ibr::Property osg_ibr::CompositeProperty" )
	{
		ADD_USER_SERIALIZER( Properties );  // _properties
	}
}

namespace osg_ibr_ScalarProperty
{
	REGISTER_OBJECT_WRAPPER( osg_ibr_ScalarProperty,
							 new osg_ibr::ScalarProperty("unknown", 0.0f),
							 osg_ibr::ScalarProperty,
							 "osg::Object osg_ibr::Property osg_ibr::ScalarProperty" )
	{
		ADD_FLOAT_SERIALIZER( Value, 1.0f );
	}
}

namespace osg_ibr_IsoSurfaceProperty
{
	REGISTER_OBJECT_WRAPPER( osg_ibr_IsoSurfaceProperty,
							 new osg_ibr::IsoSurfaceProperty,
							 osg_ibr::IsoSurfaceProperty,
							 "osg::Object osg_ibr::Property osg_ibr::ScalarProperty osg_ibr::IsoSurfaceProperty" )
	{
	}
}

namespace osg_ibr_SampleDensityProperty
{
	REGISTER_OBJECT_WRAPPER( osg_ibr_SampleDensityProperty,
							new osg_ibr::SampleDensityProperty,
							osg_ibr::SampleDensityProperty,
							"osg::Object osg_ibr::Property osg_ibr::ScalarProperty osg_ibr::SampleDensityProperty" )
	{
	}
}

namespace osg_ibr_SampleDensityWhenMovingProperty
{
	REGISTER_OBJECT_WRAPPER( osg_ibr_SampleDensityWhenMovingProperty,
							new osg_ibr::SampleDensityWhenMovingProperty,
							osg_ibr::SampleDensityWhenMovingProperty,
							"osg::Object osg_ibr::Property osg_ibr::ScalarProperty osg_ibr::SampleDensityWhenMovingProperty" )
	{
	}
}

namespace osg_ibr_SampleDensityJitterProperty {
	REGISTER_OBJECT_WRAPPER(osg_ibr_SampleDensityJitterProperty,
							new osg_ibr::SampleDensityJitterProperty,
							osg_ibr::SampleDensityJitterProperty,
							"osg::Object osg_ibr::Property osg_ibr::ScalarProperty osg_ibr::SampleDensityJitterProperty") {}
}

namespace osg_ibr_Property
{
	REGISTER_OBJECT_WRAPPER( osg_ibr_Property,
							 new osg_ibr::Property,
							 osg_ibr::Property,
							 "osg::Object osg_ibr::Property" )
	{
	}
}


namespace osg_ibr_MaximumIntensityProjectionProperty
{
	REGISTER_OBJECT_WRAPPER( osg_ibr_MaximumIntensityProjectionProperty,
							 new osg_ibr::MaximumIntensityProjectionProperty,
							 osg_ibr::MaximumIntensityProjectionProperty,
							 "osg::Object osg_ibr::Property osg_ibr::MaximumIntensityProjectionProperty" )
	{
	}
}


namespace osg_ibr_LightingProperty
{
	REGISTER_OBJECT_WRAPPER( osg_ibr_LightingProperty,
							 new osg_ibr::LightingProperty,
							 osg_ibr::LightingProperty,
							 "osg::Object osg_ibr::Property osg_ibr::LightingProperty" )
	{
	}
}

namespace osg_ibr_SwitchProperty
{
	REGISTER_OBJECT_WRAPPER( osg_ibr_SwitchProperty,
							 new osg_ibr::SwitchProperty,
							 osg_ibr::SwitchProperty,
							 "osg::Object osg_ibr::Property osg_ibr::CompositeProperty osg_ibr::SwitchProperty" )
	{
		ADD_INT_SERIALIZER( ActiveProperty, 0 );  // _activeProperty
	}
}

namespace osg_ibr_TransferFunctionProperty
{
	REGISTER_OBJECT_WRAPPER( osg_ibr_TransferFunctionProperty,
							 new osg_ibr::TransferFunctionProperty,
							 osg_ibr::TransferFunctionProperty,
							 "osg::Object osgVolume::Property osg_ibr::TransferFunctionProperty" )
	{
		ADD_OBJECT_SERIALIZER( TransferFunction, osg::TransferFunction, NULL );  // _tf
	}
}

namespace osg_ibr_TransferFunctionGradientProperty
{
	REGISTER_OBJECT_WRAPPER( osg_ibr_TransferFunctionGradientProperty,
							 new osg_ibr::TransferFunctionGradientProperty,
							 osg_ibr::TransferFunctionGradientProperty,
							 "osg::Object osgVolume::Property osg_ibr::TransferFunctionGradientProperty" )
	{
		ADD_OBJECT_SERIALIZER( TransferFunction, osg_ibr::GradientTransferFunction, NULL );  // _tf
	}
}

namespace osg_ibr_TransferFunctionGradientLightingProperty
{
	REGISTER_OBJECT_WRAPPER( osg_ibr_TransferFunctionGradientLightingProperty,
							 new osg_ibr::TransferFunctionGradientLightingProperty,
							 osg_ibr::TransferFunctionGradientLightingProperty,
							 "osg::Object osgVolume::Property osg_ibr::TransferFunctionGradientLightingProperty" )
	{
		ADD_OBJECT_SERIALIZER( TransferFunction, osg_ibr::GradientTransferFunction, NULL );  // _tf
	}
}
