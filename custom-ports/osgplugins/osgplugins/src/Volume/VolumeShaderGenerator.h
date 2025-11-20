#ifndef VOLUMESHADERGENERATOR_H
#define VOLUMESHADERGENERATOR_H

#include <osg/Program>
#include "Volume.h"

namespace osg_ibr {

	class VolumeShaderGenerator
	{
	public:
		static osg::ref_ptr<osg::Program> generateProgram(Volume::ShadingModel shadingModel);
	};
}

#endif