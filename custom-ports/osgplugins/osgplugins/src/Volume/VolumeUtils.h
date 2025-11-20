#ifndef OSGVOLUME_VOLUMEUTILS
#define OSGVOLUME_VOLUMEUTILS

#include "Volume/Volume.h"
#include "Volume/Property.h"
#include "GradientTransferFunction.h"
#include "RayTracedTechnique.h"


namespace osgRC{

	class VolumeInfoGatherer : public osg::NodeVisitor{
	public:
		VolumeInfoGatherer():NodeVisitor(TRAVERSE_ALL_CHILDREN)
		{
		}

		virtual void apply(osg::Node& grp)
		{
			if( dynamic_cast<osg_ibr::Volume*>(&grp) )
				apply( static_cast<osg_ibr::Volume&>(grp) );
			else
				NodeVisitor::apply(static_cast<osg::Node&>(grp));
		}
		virtual void apply(osg_ibr::Volume &volume)
		{
			_volumeTechnique = volume.getVolumeTechnique();
            if(!volume.getVolumeList().empty()){
                _volumeImage = volume.getVolumeList().front();
            }
		}
		osg::ref_ptr<osg::Image> _volumeImage;
		osg::ref_ptr<osg_ibr::RayTracedTechnique> _volumeTechnique;
	};


	class VolumeFunctionGatherer : public osg::NodeVisitor{
	public:
		VolumeFunctionGatherer():
			NodeVisitor(TRAVERSE_ALL_CHILDREN),
			_gotResult(false),
			_isoThreshold(0.5f),
			_sampleDensity(0.05f),
			_sampleDensityWhenMoving(0.1f)
		{
		}

		virtual void apply(osg::Node& grp)
		{
			if( dynamic_cast<osg_ibr::Volume*>(&grp) )
				apply( static_cast<osg_ibr::Volume&>(grp) );
			else
			  NodeVisitor::apply(static_cast<osg::Node&>(grp));
		}
		virtual void apply(osg_ibr::Volume &volume)
		{
			if(!_gotResult){
                if(volume.getProperty()){
					_gotResult = true;
					osg_ibr::CollectPropertiesVisitor cpv;
                    volume.getProperty()->accept(cpv);
					if(cpv._isoProperty.valid()){
						_renderingMethod = 1;
						_isoThreshold = cpv._isoProperty->getValue();
					} else if(cpv._mipProperty.valid()){
						_renderingMethod = 2;
					} else if(cpv._lightingProperty.valid()){
						_renderingMethod = 3;
					} else if(cpv._transferFunctionGradientProperty.valid()){
						osg_ibr::GradientTransferFunction *tf = dynamic_cast<osg_ibr::GradientTransferFunction*>(cpv._transferFunctionGradientProperty->getTransferFunction());
						if(tf){
							_quadMap = tf->getColorMap();
						}
						_renderingMethod = 4;
					} else if(cpv._transferFunctionGradientLightingProperty.valid()){
						osg_ibr::GradientTransferFunction *tf = dynamic_cast<osg_ibr::GradientTransferFunction*>(cpv._transferFunctionGradientLightingProperty->getTransferFunction());
						if(tf){
							_quadMap = tf->getColorMap();
						}
						_renderingMethod = 5;
					} else {
						_renderingMethod = 0;
					}
					if(cpv._sampleDensityProperty.valid()){
						_sampleDensity = cpv._sampleDensityProperty->getValue();
					}
					if(cpv._sampleDensityWhenMovingProperty.valid()){
						_sampleDensityWhenMoving = cpv._sampleDensityWhenMovingProperty->getValue();
					}
					if(cpv._tfProperty.valid()){
						osg::TransferFunction1D *tf = dynamic_cast<osg::TransferFunction1D*>(cpv._tfProperty->getTransferFunction());
						if(tf){
							const osg::TransferFunction1D::ColorMap &colorMap = tf->getColorMap();
							for(osg::TransferFunction1D::ColorMap::const_iterator it = colorMap.begin(); it != colorMap.end();++it){
								_points.push_back(it->first);
								_points.push_back(it->second.x());
								_points.push_back(it->second.y());
								_points.push_back(it->second.z());
								_points.push_back(it->second.w());
							}
						}
					}
					if(cpv._sampleDensityJitterProperty.valid()){
						_sampleJitter = cpv._sampleDensityJitterProperty->getValue();
					}
				}
			}
		}
		bool	_gotResult;
		int		_renderingMethod;
		float	_isoThreshold;
		float	_sampleDensity;
		float	_sampleDensityWhenMoving;
		float   _sampleJitter;
		std::vector<float> _points;
		osg_ibr::GradientTransferFunction::QuadMap _quadMap;
	};

	class VolumeFunctionChangeVisitor: public osg::NodeVisitor{
	public:
		VolumeFunctionChangeVisitor(int renderingMethod, float isoThreshold, float sampleDensity, float sampleDensityWhenMoving, float sampleJitter, const std::vector<float> &points, const osg_ibr::GradientTransferFunction::QuadMap &quadMap):
			NodeVisitor(TRAVERSE_ACTIVE_CHILDREN)
		{
			_property = new osg_ibr::CompositeProperty();
			_property->addProperty(new osg_ibr::SampleDensityProperty(osg::clampAbove(sampleDensity,0.0005f)));
			_property->addProperty(new osg_ibr::SampleDensityWhenMovingProperty(osg::clampAbove(sampleDensityWhenMoving,0.0005f)));
			_property->addProperty(new osg_ibr::SampleDensityJitterProperty(osg::clampBetween(sampleJitter, 0.0f, 1.0f)));
			osg::TransferFunction1D::ColorMap colorMap;
			if(renderingMethod == 1){
				_property->addProperty(new osg_ibr::IsoSurfaceProperty(isoThreshold));
			} else if(renderingMethod == 2){
				_property->addProperty(new osg_ibr::MaximumIntensityProjectionProperty());
			} else if(renderingMethod == 3){
				_property->addProperty(new osg_ibr::LightingProperty());
			} else if(renderingMethod == 4){
				osg_ibr::GradientTransferFunction* gtf = new osg_ibr::GradientTransferFunction();
				gtf->assign(quadMap,false);
				_property->addProperty(new osg_ibr::TransferFunctionGradientProperty(gtf));
			} else if(renderingMethod == 5){
				osg_ibr::GradientTransferFunction* gtf = new osg_ibr::GradientTransferFunction();
				gtf->assign(quadMap,false);
				_property->addProperty(new osg_ibr::TransferFunctionGradientLightingProperty(gtf));
			}
			if(points.size() >= 5){
				for(unsigned int i = 0; i < points.size()-4;i+=5){
					colorMap[points[i]] = osg::Vec4(points[i+1],points[i+2],points[i+3],points[i+4]);
				}
				osg::TransferFunction1D *tf = new osg::TransferFunction1D();
				tf->assign(colorMap);
				_property->addProperty(new osg_ibr::TransferFunctionProperty(tf));
			}
		}

		virtual void apply(osg::Node& grp)
		{
			if( dynamic_cast<osg_ibr::Volume*>(&grp) )
				apply( static_cast<osg_ibr::Volume&>(grp) );
			else
			  NodeVisitor::apply(static_cast<osg::Node&>(grp));
		}
		virtual void apply(osg_ibr::Volume &volume)
		{
            volume.setProperty(_property.get());
		    volume.setDirty(true);
		}
	private:
		osg::ref_ptr<osg_ibr::CompositeProperty> _property;
	};
}

#endif