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

#include "RayTracedTechnique.h"
#include "Volume.h"
#include "Property.h"

#include <osg/Geometry>
#include <osg/io_utils>

#include <osg/Program>
#include <osg/TexGen>
#include <osg/Texture1D>
#include <osg/Texture2D>
#include <osg/Texture3D>
#include <osg/TransferFunction>
#include <osg/CullFace>

#include <osgDB/ReadFile>
#include <osgDB/WriteFile>

#include <osgDB/ReadFile>
#include <osgDB/Registry>
#include <osgDB/Input>
#include <osgDB/Output>
#include <osgDB/ParameterOutput>
#include <osgDB/ObjectWrapper>
#include <osgDB/InputStream>
#include <osgDB/OutputStream>
#include <osg/ImageSequence>

#include "VolumeShaderGenerator.h"

#include <osg/ImageUtils>

using namespace osg_ibr;

RayTracedTechnique::RayTracedTechnique():
    _volume(0),
	_initializedOnce(false)
{
	_gradientImage = new osg::Image;
	_gradientImageSmooth = new osg::Image;
	_gradientLengthHistogram = new osg::Image;
	_gradientTransferFunction = new osg_ibr::GradientTransferFunction();
	_clipPlaneAlpha = new osg::Uniform("ClipPlaneAlpha",0.95f);
	_clipPlanNormal = new osg::Uniform("ClipPlaneNormal",osg::Vec3(0,0,1));
	_clipPlanD = new osg::Uniform("ClipPlaneD",1.1f);
}

RayTracedTechnique::RayTracedTechnique(const RayTracedTechnique& fft,const osg::CopyOp& copyop):
    osg::Object(fft,copyop),
    _volume(0),
	_initializedOnce(false)
{
	_gradientImage = new osg::Image;
	_gradientImageSmooth = new osg::Image;
	_gradientLengthHistogram = new osg::Image;
	_clipPlaneAlpha = new osg::Uniform("ClipPlaneAlpha",0.95f);
	_clipPlanNormal = new osg::Uniform("ClipPlaneNormal",osg::Vec3(0,0,1));
	_clipPlanD = new osg::Uniform("ClipPlaneD",1.1f);
}

RayTracedTechnique::~RayTracedTechnique()
{
}

void RayTracedTechnique::CalculateGradient(const osg::Image &input, osg::Image &output){
    osg::ref_ptr<osg::Image> gradientImageTmp = new osg::Image;
    osg::Image &gradientImage = *(gradientImageTmp.get());
	gradientImage.allocateImage(input.s(),input.t(),input.r(),GL_RGBA,GL_FLOAT); // use floats for better precision for intermediate calculations
    output.allocateImage(input.s(), input.t(), input.r(), GL_RGBA, GL_UNSIGNED_BYTE); // final will be quantized to 8 bits per channel
    if(gradientImage.data() == NULL){
        return;
    }
    // set first slice to zero
	memset(gradientImage.data(), 0, gradientImage.getImageSizeInBytes());//clear single 2D image
//#define TIME_CALCGRADIENT_GENERATOR 1
#if TIME_CALCGRADIENT_GENERATOR
	osg::Timer_t startCalcGradient = osg::Timer::instance()->tick();
#endif
	for(int r = 1; r < input.r()-1; r++){
		memset(gradientImage.data(0, 0, r), 0, gradientImage.getRowSizeInBytes());//clear first row of pixels
		for(int t = 1; t < input.t()-1; t++){
			((osg::Vec4*)gradientImage.data(0,t,r))->set(0,0,0,0);//clear first pixel
			//left the slow code here because it's simpler to modify than the unrolled version below, (about 10 times faster)
#ifdef SLOW_GRADIENT_GENERATOR
            // cache values on the stack so we safe one call to getColor in the inner loop
            float cSl = input.getColor(0, t, r).x();
            float cSm = input.getColor(1, t, r).x();
			for(int s = 1; s < input.s()-1; s++){
                float cSt = input.getColor(s+1, t, r).x();
                osg::Vec3 delta = osg::Vec3(cSl -  cSt,
											input.getColor(s,t-1,r).x() -  input.getColor(s,t+1,r).x(),
											input.getColor(s,t,r-1).x() -  input.getColor(s,t,r+1).x());
				((osg::Vec3*)gradientImage.data(s,t,r))->set(delta[0],delta[1],delta[2]);
                cSl = cSm;
                cSm = cSt;
			}
#endif
			((osg::Vec4*)gradientImage.data(input.s()-1,t,r))->set(0,0,0,0);//clear last pixel
		}
		memset(gradientImage.data(0, input.t() - 1, r), 0, gradientImage.getRowSizeInBytes());//clear last row of pixels
	}
#ifndef SLOW_GRADIENT_GENERATOR
	switch (input.getDataType())
	{
	default:
		//warn unknown data type
		OSG_WARN << input.getFileName().c_str() << " generating gradient: unknown data type " << input.getDataType() << " - treating data as GL_BYTE." << std::endl;
	case(GL_BYTE) :
	{
		char *data = (char *)input.data(0, 0, 0);
		unsigned int dataRStep = (char *)input.data(0, 0, 1) - data;
		unsigned int dataTStep = (char *)input.data(0, 1, 0) - data;
		for (int r = 1; r < input.r() - 1; r++) for (int t = 1; t < input.t() - 1; t++){
			data = (char *)input.data(0, t, r);
			float cSl = *(data);
			float cSm = *(data+1);
			for (int s = 1; s < input.s() - 1; s++){
				data += 1;
				float cSt = *(data + 1);
				osg::Vec3 delta = osg::Vec3(cSl - cSt,
					*(data - dataTStep) - *(data + dataTStep),
					*(data - dataRStep) - *(data + dataRStep));
				((osg::Vec3*)gradientImage.data(s, t, r))->set(delta[0] / 128.0f, delta[1] / 128.0f, delta[2] / 128.0f);
				cSl = cSm; cSm = cSt;
			}
		}
		break;
	}
	case(GL_UNSIGNED_BYTE) :
	{
		unsigned char *data = (unsigned char *)input.data(0, 0, 0);
		unsigned int dataRStep = (unsigned char *)input.data(0, 0, 1) - data;
		unsigned int dataTStep = (unsigned char *)input.data(0, 1, 0) - data;
		for (int r = 1; r < input.r() - 1; r++) for (int t = 1; t < input.t() - 1; t++){
			data = (unsigned char *)input.data(0, t, r);
			float cSl = *(data);
			float cSm = *(data + 1);
			for (int s = 1; s < input.s() - 1; s++){
				data += 1;
				float cSt = *(data + 1);
				osg::Vec3 delta = osg::Vec3(cSl - cSt,
					*(data - dataTStep) - *(data + dataTStep),
					*(data - dataRStep) - *(data + dataRStep));
				((osg::Vec3*)gradientImage.data(s, t, r))->set(delta[0] / 255.0f, delta[1] / 255.0f, delta[2] / 255.0f);
				cSl = cSm; cSm = cSt;
			}
		}
		break;
	}
	case(GL_SHORT) :
	{
		short *data = (short *)input.data(0, 0, 0);
		unsigned int dataRStep = (short *)input.data(0, 0, 1) - data;
		unsigned int dataTStep = (short *)input.data(0, 1, 0) - data;
		for (int r = 1; r < input.r() - 1; r++) for (int t = 1; t < input.t() - 1; t++){
			data = (short *)input.data(0, t, r);
			float cSl = *(data);
			float cSm = *(data + 1);
			for (int s = 1; s < input.s() - 1; s++){
				data += 1;
				float cSt = *(data + 1);
				osg::Vec3 delta = osg::Vec3(cSl - cSt,
					*(data - dataTStep) - *(data + dataTStep),
					*(data - dataRStep) - *(data + dataRStep));
				((osg::Vec3*)gradientImage.data(s, t, r))->set(delta[0] / 32768.0f, delta[1] / 32768.0f, delta[2] / 32768.0f);
				cSl = cSm; cSm = cSt;
			}
		}
		break;
	}
	case(GL_UNSIGNED_SHORT) :
	{
		unsigned short *data = (unsigned short *)input.data(0, 0, 0);
		unsigned int dataRStep = (unsigned short *)input.data(0, 0, 1) - data;
		unsigned int dataTStep = (unsigned short *)input.data(0, 1, 0) - data;
		for (int r = 1; r < input.r() - 1; r++) for (int t = 1; t < input.t() - 1; t++){
			data = (unsigned short *)input.data(0, t, r);
			float cSl = *(data);
			float cSm = *(data + 1);
			for (int s = 1; s < input.s() - 1; s++){
				data += 1;
				float cSt = *(data + 1);
				osg::Vec3 delta = osg::Vec3(cSl - cSt,
					*(data - dataTStep) - *(data + dataTStep),
					*(data - dataRStep) - *(data + dataRStep));
				((osg::Vec3*)gradientImage.data(s, t, r))->set(delta[0] / 65535.0f, delta[1] / 65535.0f, delta[2] / 65535.0f);
				cSl = cSm; cSm = cSt;
			}
		}
		break;
	}	
	case(GL_INT) :
	{
		int *data = (int *)input.data(0, 0, 0);
		unsigned int dataRStep = (int *)input.data(0, 0, 1) - data;
		unsigned int dataTStep = (int *)input.data(0, 1, 0) - data;
		for (int r = 1; r < input.r() - 1; r++) for (int t = 1; t < input.t() - 1; t++){
			data = (int *)input.data(0, t, r);
			float cSl = *(data);
			float cSm = *(data + 1);
			for (int s = 1; s < input.s() - 1; s++){
				data += 1;
				float cSt = *(data + 1);
				osg::Vec3 delta = osg::Vec3(cSl - cSt,
					*(data - dataTStep) - *(data + dataTStep),
					*(data - dataRStep) - *(data + dataRStep));
				((osg::Vec3*)gradientImage.data(s, t, r))->set(delta[0] / 2147483648.0f, delta[1] / 2147483648.0f, delta[2] / 2147483648.0f);
				cSl = cSm; cSm = cSt;
			}
		}
		break;
	}
	case(GL_UNSIGNED_INT) :
	{
		unsigned int *data = (unsigned int *)input.data(0, 0, 0);
		unsigned int dataRStep = (unsigned int *)input.data(0, 0, 1) - data;
		unsigned int dataTStep = (unsigned int *)input.data(0, 1, 0) - data;
		for (int r = 1; r < input.r() - 1; r++) for (int t = 1; t < input.t() - 1; t++){
			data = (unsigned int *)input.data(0, t, r);
			float cSl = *(data);
			float cSm = *(data + 1);
			for (int s = 1; s < input.s() - 1; s++){
				data += 1;
				float cSt = *(data + 1);
				osg::Vec3 delta = osg::Vec3(cSl - cSt,
					*(data - dataTStep) - *(data + dataTStep),
					*(data - dataRStep) - *(data + dataRStep));
				((osg::Vec3*)gradientImage.data(s, t, r))->set(delta[0] / 4294967295.0f, delta[1] / 4294967295.0f, delta[2] / 4294967295.0f);
				cSl = cSm; cSm = cSt;
			}
		}
		break;
	}

	case(GL_FLOAT) :
	{
		float *data = (float *)input.data(0, 0, 0);
		unsigned int dataRStep = (float *)input.data(0, 0, 1) - data;
		unsigned int dataTStep = (float *)input.data(0, 1, 0) - data;
		for (int r = 1; r < input.r() - 1; r++) for (int t = 1; t < input.t() - 1; t++){
			data = (float *)input.data(0, t, r);
			float cSl = *(data);
			float cSm = *(data + 1);
			for (int s = 1; s < input.s() - 1; s++){
				data += 1;
				float cSt = *(data + 1);
				osg::Vec3 delta = osg::Vec3(cSl - cSt,
					*(data - dataTStep) - *(data + dataTStep),
					*(data - dataRStep) - *(data + dataRStep));
				((osg::Vec3*)gradientImage.data(s, t, r))->set(delta[0], delta[1], delta[2]);
				cSl = cSm; cSm = cSt;
			}
		}
		break;
	}
	case(GL_DOUBLE) :
	{
		double *data = (double *)input.data(0, 0, 0);
		unsigned int dataRStep = (double *)input.data(0, 0, 1) - data;
		unsigned int dataTStep = (double *)input.data(0, 1, 0) - data;
		for (int r = 1; r < input.r() - 1; r++) for (int t = 1; t < input.t() - 1; t++){
			data = (double *)input.data(0, t, r);
			float cSl = *(data);
			float cSm = *(data + 1);
			for (int s = 1; s < input.s() - 1; s++){
				data += 1;
				float cSt = *(data + 1);
				osg::Vec3 delta = osg::Vec3(cSl - cSt,
					*(data - dataTStep) - *(data + dataTStep),
					*(data - dataRStep) - *(data + dataRStep));
				((osg::Vec3*)gradientImage.data(s, t, r))->set(delta[0], delta[1], delta[2]);
				cSl = cSm; cSm = cSt;
			}
		}
		break;
	}

	}
#endif
    // set last slice to zero
	memset(gradientImage.data(0, 0, input.r() - 1), 0, gradientImage.getImageSizeInBytes());//clear single 2D image
#if TIME_CALCGRADIENT_GENERATOR
	osg::Timer_t doneCalcGradient = osg::Timer::instance()->tick();
	OSG_NOTICE << input.getFileName().c_str() << " generated gradient: " << osg::Timer::instance()->delta_m(startCalcGradient, doneCalcGradient) << " ms." << std::endl;
#endif
	// now smooth everything
	osg::Image &gradientImageSmooth = *(_gradientImageSmooth.get());
	gradientImageSmooth.allocateImage(input.s(),input.t(),input.r(),GL_RGBA,GL_FLOAT);
	// first zero out
	memset(gradientImageSmooth.data(), 0, gradientImageSmooth.getTotalSizeInBytes());
	// calculate the gradient
	float w0 = 1.0f;
	float w1 = 0.3f;
	float w2 = 0.2f;
	float w3 = 0.1f;
	float div = 1.0f/(w0 + 6*w1 + 12*w2 + 8*w3);
	//optimize: premultiply weight factors
	w0 *= div;
	w1 *= div;
	w2 *= div;
	w3 *= div;
	float maxLength = FLT_MIN;
//#define TIME_GRADIENT_GENERATOR 1
#if TIME_GRADIENT_GENERATOR
	osg::Timer_t startGradient = osg::Timer::instance()->tick();
#endif
	for (int r = 1; r < input.r() - 1; r++){
		for (int t = 1; t < input.t() - 1; t++){
			// we cache some data in the registers / on the stack
			// this way we only need to read 9 new values instead of 27 each interation
			// shaves 20 seconds of the loading time.
			osg::Vec3 a0 = *(osg::Vec3*)gradientImage.data(0, t, r);
			osg::Vec3 a1 = *(osg::Vec3*)gradientImage.data(0, t - 1, r);
			a1 += *(osg::Vec3*)gradientImage.data(0, t + 1, r);
			a1 += *(osg::Vec3*)gradientImage.data(0, t, r + 1);
			a1 += *(osg::Vec3*)gradientImage.data(0, t, r - 1);
			osg::Vec3 a2 = *(osg::Vec3*)gradientImage.data(0, t - 1, r - 1);
			a2 += *(osg::Vec3*)gradientImage.data(0, t + 1, r - 1);
			a2 += *(osg::Vec3*)gradientImage.data(0, t - 1, r + 1);
			a2 += *(osg::Vec3*)gradientImage.data(0, t + 1, r + 1);
			osg::Vec3 b0 = *(osg::Vec3*)gradientImage.data(1, t, r);
			osg::Vec3 b1 = *(osg::Vec3*)gradientImage.data(1, t - 1, r);
			b1 += *(osg::Vec3*)gradientImage.data(1, t + 1, r);
			b1 += *(osg::Vec3*)gradientImage.data(1, t, r + 1);
			b1 += *(osg::Vec3*)gradientImage.data(1, t, r - 1);
			osg::Vec3 b2 = *(osg::Vec3*)gradientImage.data(1, t - 1, r - 1);
			b2 += *(osg::Vec3*)gradientImage.data(1, t + 1, r - 1);
			b2 += *(osg::Vec3*)gradientImage.data(1, t - 1, r + 1);
			b2 += *(osg::Vec3*)gradientImage.data(1, t + 1, r + 1);
			osg::Vec3 aF = (a2 * w3) + (a1 * w2) + (a0 * w1);
			osg::Vec3 bF = (b2 * w3) + (b1 * w2) + (b0 * w1);
			osg::Vec3 bN = (b2 * w2) + (b1 * w1) + (b0 * w0);

			osg::Vec4 *smoothData = (osg::Vec4*)gradientImageSmooth.data(0, t, r);
			osg::Vec3* data = (osg::Vec3*)gradientImage.data(1, t - 1, r - 1);
			unsigned int dataRStep = (osg::Vec3*)gradientImage.data(1, t - 1, r) - data;
			unsigned int dataTStep = (osg::Vec3*)gradientImage.data(1, t, r - 1) - data;

			for (int s = 1; s < input.s() - 1; s++) {
				smoothData += 1;
				osg::Vec3 cF, cN;
				{
					data = (osg::Vec3*) (((osg::Vec4*)data) + 1);
					osg::Vec3 c0, c1, c2;
					c2 = *(data);
					c1 = *(data + dataTStep);
					c2 += *(data + 2 * dataTStep);
					osg::Vec3* Rdata = data + dataRStep;
					c1 += *(Rdata);
					c0 = *(Rdata + dataTStep);
					c1 += *(Rdata + 2 * dataTStep);
					Rdata = data + 2 * dataRStep;
					c2 += *(Rdata);
					c1 += *(Rdata + dataTStep);
					c2 += *(Rdata + 2 * dataTStep);
					cF = (c2 * w3) + (c1 * w2) + (c0 * w1);
					cN = (c2 * w2) + (c1 * w1) + (c0 * w0);
				}
				osg::Vec3 dataTmp = aF + bN + cF;

				float norm = ((osg::Vec3*)&dataTmp)->normalize();
				maxLength = osg::maximum(norm, maxLength);
				smoothData->set((dataTmp[0] * 0.5f + 0.5f), (dataTmp[1] * 0.5f + 0.5f), (dataTmp[2] * 0.5f + 0.5f), norm);
				aF = bF;
				bN = cN;
				bF = cF;
			}
		}
	}
#if TIME_GRADIENT_GENERATOR
	osg::Timer_t doneGradient = osg::Timer::instance()->tick();
	OSG_NOTICE << input.getFileName().c_str() << " blurred gradient: " << osg::Timer::instance()->delta_m(startGradient, doneGradient) << " ms." << std::endl;
#if 0
	for(int r = 1; r < input.r()-1; r++){
		for(int t = 1; t < input.t()-1; t++){
            // we cache some data in the registers / on the stack
            // this way we only need to read 9 new values instead of 27 each interation
            // shaves 20 seconds of the loading time.
            osg::Vec4 sta = *(osg::Vec4*)gradientImage.data(0, t-1, r-1);
            osg::Vec4 stb = *(osg::Vec4*)gradientImage.data(1, t-1, r-1);
            osg::Vec4 sma = *(osg::Vec4*)gradientImage.data(0, t  , r-1);
            osg::Vec4 smb = *(osg::Vec4*)gradientImage.data(1, t  , r-1);
            osg::Vec4 sla = *(osg::Vec4*)gradientImage.data(0, t+1, r-1);
            osg::Vec4 slb = *(osg::Vec4*)gradientImage.data(1, t+1, r-1);

            osg::Vec4 s2ta = *(osg::Vec4*)gradientImage.data(0, t-1, r);
            osg::Vec4 s2tb = *(osg::Vec4*)gradientImage.data(1, t-1, r);
            osg::Vec4 s2ma = *(osg::Vec4*)gradientImage.data(0, t  , r);
            osg::Vec4 s2mb = *(osg::Vec4*)gradientImage.data(1, t  , r);
            osg::Vec4 s2la = *(osg::Vec4*)gradientImage.data(0, t+1, r);
            osg::Vec4 s2lb = *(osg::Vec4*)gradientImage.data(1, t+1, r);

            osg::Vec4 s3ta = *(osg::Vec4*)gradientImage.data(0, t-1, r+1);
            osg::Vec4 s3tb = *(osg::Vec4*)gradientImage.data(1, t-1, r+1);
            osg::Vec4 s3ma = *(osg::Vec4*)gradientImage.data(0, t  , r+1);
            osg::Vec4 s3mb = *(osg::Vec4*)gradientImage.data(1, t  , r+1);
            osg::Vec4 s3la = *(osg::Vec4*)gradientImage.data(0, t+1, r+1);
            osg::Vec4 s3lb = *(osg::Vec4*)gradientImage.data(1, t+1, r+1);

			for(int s = 1; s < input.s()-1; s++){
                osg::Vec4 stc = *(osg::Vec4*)gradientImage.data(s+1, t-1, r-1);
                osg::Vec4 smc = *(osg::Vec4*)gradientImage.data(s+1, t  , r-1);
                osg::Vec4 slc = *(osg::Vec4*)gradientImage.data(s+1, t+1, r-1);

                osg::Vec4 s2tc = *(osg::Vec4*)gradientImage.data(s+1, t-1, r);
                osg::Vec4 s2mc = *(osg::Vec4*)gradientImage.data(s+1, t  , r);
                osg::Vec4 s2lc = *(osg::Vec4*)gradientImage.data(s+1, t+1, r);

                osg::Vec4 s3tc = *(osg::Vec4*)gradientImage.data(s+1, t-1, r+1);
                osg::Vec4 s3mc = *(osg::Vec4*)gradientImage.data(s+1, t  , r+1);
                osg::Vec4 s3lc = *(osg::Vec4*)gradientImage.data(s+1, t+1, r+1);

				osg::Vec4 *data = (osg::Vec4*)gradientImageSmooth.data(s,t,r);

				osg::Vec4 dataTmp = (sta + stc + sla + slc + s3ta + s3tc + s3la + s3lc) * w3 +
					                (stb + sma + smc + slb + s2ta + s2tc + s2la + s2lc + s3tb + s3ma + s3mc + s3lb ) * w2 +
					                (smb + s2tb + s2ma + s2mc + s2lb + s3mb) * w1 +
					                s2mb * w0;

                float norm = ((osg::Vec3*)&dataTmp)->normalize();
                maxLength = osg::maximum(norm, maxLength);
                data->set((dataTmp[0]+1.0f)*0.5f, (dataTmp[1]+1.0f)*0.5f, (dataTmp[2]+1.0f)*0.5f, norm);
                sta = stb;
                stb = stc;
                sma = smb;
                smb = smc;
                sla = slb;
                slb = slc;

                s2ta = s2tb;
                s2tb = s2tc;
                s2ma = s2mb;
                s2mb = s2mc;
                s2la = s2lb;
                s2lb = s2lc;

                s3ta = s3tb;
                s3tb = s3tc;
                s3ma = s3mb;
                s3mb = s3mc;
                s3la = s3lb;
                s3lb = s3lc;
			}
		}
	}
	osg::Timer_t doneGradientAgain = osg::Timer::instance()->tick();
	OSG_NOTICE << input.getFileName().c_str() << " blurred gradient Again: " << osg::Timer::instance()->delta_m(doneGradient, doneGradientAgain) << " ms." << std::endl;

#endif
#endif
	float norm = 1.0f / (maxLength + 0.01f);
    osg::Image &histogram = *(_gradientLengthHistogram.get());
    int alphaScale = 256;
    int lengthScale = 256;
    histogram.allocateImage(alphaScale, lengthScale, 1, GL_LUMINANCE, GL_FLOAT);
	memset(histogram.data(), 0, histogram.getTotalSizeInBytes());
    float maxLengthHistogram = 0.0f;
    // copy everything into the final image and convert to chars, also make histogram
    for(int r = 0; r < input.r(); r++){
        for(int t = 0; t < input.t(); t++){
            osg::Vec4 *data = (osg::Vec4*)gradientImageSmooth.data(0, t, r);
            osg::Vec4ub *data2 = (osg::Vec4ub*)output.data(0, t, r);
			const unsigned char *inputData = input.data(0, t, r);
            for(int s = 0; s < input.s(); s++){
                float length = data->w()*norm;
                data2->set(data->x()*256, data->y()*256, data->z()*256, (length)*256);

//                float alpha = input.getColor(s, t, r).x();
				float alpha;
				switch (input.getDataType()) 
				{
				default:
				case(GL_BYTE) :
					alpha = (*(char *)(inputData)) / 128.0f;
					inputData += sizeof(char);
					break;
				case(GL_UNSIGNED_BYTE) :
					alpha = (*(unsigned char *)(inputData)) / 255.0f;
					inputData += sizeof(unsigned char);
					break;
				case(GL_SHORT) :
					alpha = (*(short *)(inputData) / 32768.0f);
					inputData += sizeof(short);
					break;
				case(GL_UNSIGNED_SHORT) :
					alpha = (*(unsigned short *)(inputData)) / 65535.0f;
					inputData += sizeof(unsigned short);
					break;
				case(GL_INT) :
					alpha = (*(int *)(inputData)) / 2147483648.0f;
					inputData += sizeof(int);
					break;
				case(GL_UNSIGNED_INT) :
					alpha = (*(unsigned int  *)(inputData)) / 4294967295.0f;
					inputData += sizeof(unsigned int);
					break;
				case(GL_FLOAT) :
					alpha = (*(float *)(inputData));
					inputData += sizeof(float);
					break;
				case(GL_DOUBLE) :
					alpha = (*(double *)(inputData));
					inputData += sizeof(double);
					break;
				}
                float* dataHistogram = (float*)histogram.data(alpha*alphaScale, length*lengthScale);
                maxLengthHistogram = osg::maximum(maxLengthHistogram, *dataHistogram + 1.0f);
                *dataHistogram += 1.0f;
				data += 1;
				data2 += 1;
            }
        }
    }
	// now normalize the histogram, use a logarithm in the distribution
	norm = 100000.0f/maxLengthHistogram;
	for(int t = 0; t < histogram.t(); t++){
		float *data = (float*)histogram.data(0,t);
		for(int s = 0; s < histogram.s(); s++){
			*data = logf(*data*norm)/logf(100000.0f);
			data += 1;
		}
	}
#if TIME_CALCGRADIENT_GENERATOR
	osg::Timer_t doneCalculateGradient = osg::Timer::instance()->tick();
	OSG_NOTICE << input.getFileName().c_str() << " CalculateGradient() done in " << osg::Timer::instance()->delta_m(startCalcGradient, doneCalculateGradient) << " ms." << std::endl;
#endif
}

void RayTracedTechnique::init()
{
    OSG_INFO<<"RayTracedTechnique::init()"<<std::endl;

    if (!_volume)
    {
        OSG_WARN<<"RayTracedTechnique::init(), error no volume tile assigned."<<std::endl;
        return;
    }

    if (_volume->getVolumeList().empty())
    {
        OSG_WARN<<"RayTracedTechnique::init(), error no image assigned to layer."<<std::endl;
        return;
    }

	if(!_geode.valid()){

        _geode = new osg::Geode;

        osg::Geometry* geom = new osg::Geometry;

        osg::Vec3Array* coords = new osg::Vec3Array(8);
        (*coords)[0] = osg::Vec3d(0.0,0.0,0.0);
        (*coords)[1] = osg::Vec3d(1.0,0.0,0.0);
        (*coords)[2] = osg::Vec3d(1.0,1.0,0.0);
        (*coords)[3] = osg::Vec3d(0.0,1.0,0.0);
        (*coords)[4] = osg::Vec3d(0.0,0.0,1.0);
        (*coords)[5] = osg::Vec3d(1.0,0.0,1.0);
        (*coords)[6] = osg::Vec3d(1.0,1.0,1.0);
        (*coords)[7] = osg::Vec3d(0.0,1.0,1.0);
        geom->setVertexArray(coords);

        osg::DrawElementsUShort* drawElements = new osg::DrawElementsUShort(GL_QUADS);
        // bottom
        drawElements->push_back(0);
        drawElements->push_back(1);
        drawElements->push_back(2);
        drawElements->push_back(3);

        // bottom
        drawElements->push_back(3);
        drawElements->push_back(2);
        drawElements->push_back(6);
        drawElements->push_back(7);

        // left
        drawElements->push_back(0);
        drawElements->push_back(3);
        drawElements->push_back(7);
        drawElements->push_back(4);

        // right
        drawElements->push_back(5);
        drawElements->push_back(6);
        drawElements->push_back(2);
        drawElements->push_back(1);

        // front
        drawElements->push_back(1);
        drawElements->push_back(0);
        drawElements->push_back(4);
        drawElements->push_back(5);

        // top
        drawElements->push_back(7);
        drawElements->push_back(6);
        drawElements->push_back(5);
        drawElements->push_back(4);

        geom->addPrimitiveSet(drawElements);

        _geode->addDrawable(geom);
	}
	Volume::ShadingModel shadingModel = Volume::Isosurface;
    CollectPropertiesVisitor cpv;
    if (_volume->getProperty())
    {
        _volume->getProperty()->accept(cpv);
    }

    if (cpv._isoProperty.valid())
    {
        shadingModel = Volume::Isosurface;
    }
    else if (cpv._mipProperty.valid())
    {
        shadingModel = Volume::MaximumIntensityProjection;
    }
    else if (cpv._lightingProperty.valid())
    {
        shadingModel = Volume::Light;
    }
	else if (cpv._transferFunctionGradientProperty.valid())
    {
		shadingModel = Volume::StandardWithGradientMagnitude;
		osg_ibr::GradientTransferFunction* gtf = dynamic_cast<osg_ibr::GradientTransferFunction*>(cpv._transferFunctionGradientProperty->getTransferFunction());
		_gradientTransferFunction->allocate(gtf->getNumberImageCells());
		_gradientTransferFunction->assign(gtf->getColorMap());
    }
	else if (cpv._transferFunctionGradientLightingProperty.valid())
    {
        shadingModel = Volume::StandardWithGradientMagnitudeAndLight;
		osg_ibr::GradientTransferFunction* gtf = dynamic_cast<osg_ibr::GradientTransferFunction*>(cpv._transferFunctionGradientLightingProperty->getTransferFunction());
		_gradientTransferFunction->allocate(gtf->getNumberImageCells());
		_gradientTransferFunction->assign(gtf->getColorMap());
    }
    else if(cpv._tfProperty.valid())
    {
        shadingModel = Volume::Standard;
    } else
    {
        shadingModel = Volume::StandardNoTransferfunction;
    }
    osg::TransferFunction1D* tf = 0;
    if (cpv._tfProperty.valid())
    {
        tf = dynamic_cast<osg::TransferFunction1D*>(cpv._tfProperty->getTransferFunction());
    }

    // first the main stateset for shared stuff
    _mainStateset = new osg::StateSet();

    if(tf)
    {
        osg::ref_ptr<osg::Texture1D> tf_texture = new osg::Texture1D;
        tf_texture->setImage(tf->getImage());

        tf_texture->setResizeNonPowerOfTwoHint(false);
        tf_texture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
        tf_texture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
        tf_texture->setWrap(osg::Texture::WRAP_R, osg::Texture::CLAMP_TO_BORDER);

        _mainStateset->setTextureAttributeAndModes(1, tf_texture.get(), osg::StateAttribute::ON);
        _mainStateset->addUniform(new osg::Uniform("tfTexture", 1));
    }

    if(shadingModel==Volume::Isosurface) {
        _mainStateset->addUniform(cpv._isoProperty->getUniform());
    }
    if(cpv._sampleDensityProperty.valid()){
        _mainStateset->addUniform(cpv._sampleDensityProperty->getUniform());
    } else {
        _mainStateset->addUniform(new osg::Uniform("SampleDensityValue", 0.015f));
    }

	if(cpv._sampleDensityJitterProperty.valid()){
		_mainStateset->addUniform(cpv._sampleDensityJitterProperty->getUniform());
	} else {
		_mainStateset->addUniform(new osg::Uniform("SampleDensityJitterValue", 1.0f));
	}

    if(shadingModel==Volume::StandardWithGradientMagnitude || shadingModel==Volume::StandardWithGradientMagnitudeAndLight){
        osg::ref_ptr<osg::Texture2D> tf_texture = new osg::Texture2D;
        tf_texture->setImage(_gradientTransferFunction->getImage());

        tf_texture->setResizeNonPowerOfTwoHint(false);
        tf_texture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
        tf_texture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
        tf_texture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_BORDER);
        tf_texture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_BORDER);

        _mainStateset->setTextureAttributeAndModes(2, tf_texture.get(), osg::StateAttribute::ON);
        _mainStateset->addUniform(new osg::Uniform("tf2DTexture", 2));
    }

    // set clip plane uniform
    _mainStateset->addUniform(_clipPlaneAlpha);
    _mainStateset->addUniform(_clipPlanNormal);
    _mainStateset->addUniform(_clipPlanD);

    _mainStateset->setMode(GL_BLEND, osg::StateAttribute::ON);
    _mainStateset->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);

    _mainStateset->setMode(GL_CULL_FACE, osg::StateAttribute::ON);

    osg::CullFace *cull_face = new osg::CullFace;
    cull_face->setMode(osg::CullFace::BACK);
    _mainStateset->setAttribute(cull_face, osg::StateAttribute::ON);

    if(cpv._sampleDensityWhenMovingProperty.valid())
    {
        _whenMovingStateSet = new osg::StateSet;
        _whenMovingStateSet->addUniform(cpv._sampleDensityWhenMovingProperty->getUniform(), osg::StateAttribute::OVERRIDE | osg::StateAttribute::ON);
    }
    _mainStateset->setAttribute(VolumeShaderGenerator::generateProgram(shadingModel));
    _mainStateset->addUniform(new osg::Uniform("baseTexture", 0));
    _mainStateset->addUniform(new osg::Uniform("gradientTexture", 3));

    if(_initializedOnce){
        return;
    }
    //now a per volume stateset
    const Volume::VolumeList &volumeList = _volume->getVolumeList();
    for(unsigned int i = 0; i < volumeList.size(); i++){

        _volumeDataList.push_back(VolumeData());
        VolumeData &volumeData = _volumeDataList.back();
        volumeData._gradientImage = new osg::Image();
        volumeData._stateSet = new osg::StateSet();
        osg::Image* image_3d = volumeList[i].get();
        if(!image_3d){
            continue;
        }
        if(!_initializedOnce && !image_3d->isCompressed()){
            CalculateGradient(*image_3d, *volumeData._gradientImage);
        }

        osg::Texture::InternalFormatMode internalFormatMode = osg::Texture::USE_IMAGE_DATA_FORMAT;
        {
            // set up the 3d texture itself,
            // note, well set the filtering up so that mip mapping is disabled,
            // this is because we don't use mipmapping when ray-traycing
            osg::Texture3D* texture3D = new osg::Texture3D;
            texture3D->setResizeNonPowerOfTwoHint(false);
            texture3D->setFilter(osg::Texture3D::MIN_FILTER, osg::Texture::LINEAR);
            texture3D->setFilter(osg::Texture3D::MAG_FILTER, osg::Texture::LINEAR);
            texture3D->setWrap(osg::Texture3D::WRAP_R, osg::Texture3D::CLAMP_TO_BORDER);
            texture3D->setWrap(osg::Texture3D::WRAP_S, osg::Texture3D::CLAMP_TO_BORDER);
            texture3D->setWrap(osg::Texture3D::WRAP_T, osg::Texture3D::CLAMP_TO_BORDER);
            texture3D->setBorderColor(osg::Vec4(0.0, 0.0, 0.0, 0.0));
            if(image_3d->getPixelFormat()==GL_ALPHA ||
                image_3d->getPixelFormat()==GL_LUMINANCE)
            {
                if(image_3d->getPixelSizeInBits() > 8){
                    texture3D->setInternalFormatMode(osg::Texture3D::USE_USER_DEFINED_FORMAT);
                    texture3D->setInternalFormat(GL_INTENSITY16);
                } else {
                    texture3D->setInternalFormatMode(osg::Texture3D::USE_USER_DEFINED_FORMAT);
                    texture3D->setInternalFormat(GL_INTENSITY);
                }
            } else if(image_3d->getPixelFormat()==GL_COMPRESSED_RGBA_S3TC_DXT5_EXT){
                texture3D->setInternalFormatMode(osg::Texture3D::USE_S3TC_DXT5_COMPRESSION);
            } else
            {
                texture3D->setInternalFormatMode(internalFormatMode);
            }
            texture3D->setImage(image_3d);

            volumeData._stateSet->setTextureAttributeAndModes(0, texture3D, osg::StateAttribute::ON);

            // gradient texture
            texture3D = new osg::Texture3D;
            texture3D->setResizeNonPowerOfTwoHint(false);
            texture3D->setFilter(osg::Texture3D::MIN_FILTER, osg::Texture::LINEAR);
            texture3D->setFilter(osg::Texture3D::MAG_FILTER, osg::Texture::LINEAR);
            texture3D->setWrap(osg::Texture3D::WRAP_R, osg::Texture3D::CLAMP_TO_BORDER);
            texture3D->setWrap(osg::Texture3D::WRAP_S, osg::Texture3D::CLAMP_TO_BORDER);
            texture3D->setWrap(osg::Texture3D::WRAP_T, osg::Texture3D::CLAMP_TO_BORDER);
            texture3D->setBorderColor(osg::Vec4(0.0, 0.0, 0.0, 0.0));
            texture3D->setInternalFormatMode(osg::Texture3D::USE_IMAGE_DATA_FORMAT);
            texture3D->setImage(volumeData._gradientImage.get());

            volumeData._stateSet->setTextureAttributeAndModes(3, texture3D, osg::StateAttribute::ON);
        }
    }

	_initializedOnce = true;
}

void RayTracedTechnique::cull(osgUtil::CullVisitor* cv)
{
    if(!_geode.valid() || _volumeDataList.empty()) return;

    bool moving = false;
    if (_whenMovingStateSet.valid())
    {
        {
            OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_mutex);
            ModelViewMatrixMap::iterator itr = _modelViewMatrixMap.find(cv);
            if (itr!=_modelViewMatrixMap.end())
            {
                osg::Matrix newModelViewMatrix = *(cv->getModelViewMatrix());
                osg::Matrix& previousModelViewMatrix = itr->second;
                moving = (newModelViewMatrix != previousModelViewMatrix);

                previousModelViewMatrix = newModelViewMatrix;
            }
            else
            {
                _modelViewMatrixMap[cv] = *(cv->getModelViewMatrix());
            }
        }
    }

    int i = (int)(cv->getFrameStamp()->getReferenceTime()*_volume->getFPS()) %_volumeDataList.size();
    if(moving) {
        cv->pushStateSet(_whenMovingStateSet.get());
    }
    cv->pushStateSet(_mainStateset.get());
    cv->pushStateSet(_volumeDataList[i]._stateSet.get());
    _geode->accept(*cv);
    cv->popStateSet();
    cv->popStateSet();
    if(moving) {
        cv->popStateSet();
    }
}

void RayTracedTechnique::dirty(){
	if (_volume){
		_volume->setDirty(true);
    }
}

void RayTracedTechnique::traverse(osg::NodeVisitor& nv)
{
    if (!_volume) return;

    // if app traversal update the frame count.
    if (nv.getVisitorType()==osg::NodeVisitor::UPDATE_VISITOR)
    {
        if (_volume->getDirty()){
            _volume->init();
        }
    }
    else if (nv.getVisitorType()==osg::NodeVisitor::CULL_VISITOR)
    {
        osgUtil::CullVisitor* cv = dynamic_cast<osgUtil::CullVisitor*>(&nv);
        if (cv)
        {
            cull(cv);
            return;
        }
    }
}

void RayTracedTechnique::setClipPlane(float alpha, const osg::Plane &plane) {
    // the normal is in world space, pproject it back into our unit cube
    osg::Matrix matrix = osg::Matrix::identity();
    osg::NodePathList nodePaths = _volume->getParentalNodePaths();
    osg::NodePath nodePathToRoot;
    if (!nodePaths.empty())
    {
        nodePathToRoot = nodePaths.front();

    }
    matrix = osg::computeLocalToWorld(nodePathToRoot);
    osg::Plane plane2 = plane;
    plane2.transformProvidingInverse(matrix);
    _clipPlaneAlpha->set(alpha);
    _clipPlanNormal->set(osg::Vec3f(plane2.getNormal()));
    _clipPlanD->set((float)plane2[3]);
}
