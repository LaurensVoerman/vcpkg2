#ifndef RC_BUMPMAPPING_
#define RC_BUMPMAPPING_

#include <osg/Geometry>
#include "osg_ibr.h"

namespace osg_ibr
{

	/*
	 * Bump mapping effect based a little on the osg effect
	 * Can have multiple lights, when switching lights on/off the shader needs to be recompiled
	 * Number of lights supported depends on the hardware
	 * The shader uses the color of the diffuse texture to react to both ambient and diffuse light
	 */
	class IBRBB_EXPORT RCBumpMapping: public osg::Geometry {
    public:
        RCBumpMapping();
        RCBumpMapping(const RCBumpMapping& copy, const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY);


        META_Object(osg_ibr, RCBumpMapping);
            
               
        /** get the texture unit that contains diffuse color texture. Default is 0 */
        int getDiffuseTextureUnit() const
        {
            return _diffuse_unit;
        }
        
        /** set the texture unit that contains diffuse color texture. Default is 0 */
        void setDiffuseTextureUnit(int n)
        {
            if(_diffuse_unit != n){
                _diffuse_unit = n;
                dirty(true);
            }
        }

        /** get the texture unit that contains normal map texture. Default is 1 */
        int getNormalMapTextureUnit() const
        {
            return _normal_unit;
        }
        
        /** set the texture unit that contains normal map texture. Default is 1 */
        void setNormalMapTextureUnit(int n)
        {
             if(_normal_unit != n){
                _normal_unit = n;
                dirty(true);
             }
        }

		void setSpotLights(const std::vector<int> &lights){
            if(_spotLights != lights){
                _spotLights = lights;
                dirty(true);
            }
        }
		void setPointLights(const std::vector<int> &lights){
            if(_pointLights != lights){
                _pointLights = lights;
                dirty(true);
            }
        }
		void setDirectionalLights(const std::vector<int> &lights){
            if(_directionalLights != lights){
                _directionalLights = lights;
                dirty(true);
            }
        }

		void setInputGamma(bool v){
            if(_inputGamma != v){
                _inputGamma = v;
                dirty(true);
            }
        }
		bool getInputGamma() const{ return _inputGamma;}
		void setOutputGamma(bool v){
            if(_outPutGamma != v){
                _outPutGamma = v;
                dirty(true);
            }
        }
		bool getOutputGamma() const{ return _outPutGamma;}

        /** force rebuilding of techniques on next traversal */
        void dirty(bool val);      

        void rebuildShaders();

    protected:
        virtual ~RCBumpMapping() {}
        RCBumpMapping &operator=(const RCBumpMapping &) { return *this; }

    private:
        int _diffuse_unit;
        int _normal_unit;
		std::vector<int> _spotLights;
		std::vector<int> _pointLights;
		std::vector<int> _directionalLights;
		bool _inputGamma; // tells if the input needs to be gamma corrected (gamma of 2.2)
		bool _outPutGamma; // tells to write the output gamma corrected
    };
}

#endif
