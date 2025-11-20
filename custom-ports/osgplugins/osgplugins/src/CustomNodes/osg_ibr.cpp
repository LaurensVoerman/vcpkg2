#include <osg/TexGen>
#include <osgUtil/CullVisitor>
#include <osgDB/Registry>
#include <osgDB/ObjectWrapper>

#include <osg/StateSet>
#include "osg_ibr.h"

#ifndef M_1_PI
#define M_1_PI	 0.318309886183790671538
#endif



namespace osg_ibr {
	class IBRBB_EXPORT ibrCallback :  public osg::NodeCallback
{
	public:
		ibrCallback():_leftEyeStateset(new osg::StateSet),_rightEyeStateset(new osg::StateSet){
			osg::TexGen* transformTexgenLeft = new osg::TexGen;
			transformTexgenLeft->setDataVariance(osg::Object::DYNAMIC); // This is dynamic because we change this in the cull
			transformTexgenLeft->setMode(osg::TexGen::OBJECT_LINEAR);
			transformTexgenLeft->setPlane(osg::TexGen::R, osg::Vec4(0.0f,0.0f,0.0f,0.2f));
			_leftEyeStateset->setDataVariance(osg::Object::DYNAMIC);
			_leftEyeStateset->setTextureAttribute(0,transformTexgenLeft);

			osg::TexGen* transformTexgenRight = new osg::TexGen;
			transformTexgenRight->setDataVariance(osg::Object::DYNAMIC);
			transformTexgenRight->setMode(osg::TexGen::OBJECT_LINEAR);
			transformTexgenRight->setPlane(osg::TexGen::R, osg::Vec4(0.0f,0.0f,0.0f,0.2f));
			_rightEyeStateset->setDataVariance(osg::Object::DYNAMIC);
			_rightEyeStateset->setTextureAttribute(0,transformTexgenRight);
		}
		ibrCallback(const ibrCallback&,const osg::CopyOp&):_leftEyeStateset(new osg::StateSet),_rightEyeStateset(new osg::StateSet) {
			osg::TexGen* transformTexgenLeft = new osg::TexGen;
			transformTexgenLeft->setDataVariance(osg::Object::DYNAMIC);
			transformTexgenLeft->setMode(osg::TexGen::OBJECT_LINEAR);
			transformTexgenLeft->setPlane(osg::TexGen::R, osg::Vec4(0.0f,0.0f,0.0f,0.2f));
			_leftEyeStateset->setDataVariance(osg::Object::DYNAMIC);
			_leftEyeStateset->setTextureAttribute(0,transformTexgenLeft);

			osg::TexGen* transformTexgenRight = new osg::TexGen;
			transformTexgenRight->setDataVariance(osg::Object::DYNAMIC);
			transformTexgenRight->setMode(osg::TexGen::OBJECT_LINEAR);
			transformTexgenRight->setPlane(osg::TexGen::R, osg::Vec4(0.0f,0.0f,0.0f,0.2f));
			_rightEyeStateset->setDataVariance(osg::Object::DYNAMIC);
			_rightEyeStateset->setTextureAttribute(0,transformTexgenRight);
		}
		META_Object(osg_ibr,ibrCallback);
		virtual void operator()(osg::Node* node, osg::NodeVisitor* nv);
	private:
		osg::ref_ptr<osg::StateSet> _leftEyeStateset;
		osg::ref_ptr<osg::StateSet> _rightEyeStateset;
	};
}

void osg_ibr::ibrCallback::operator()(osg::Node* node, osg::NodeVisitor* nv)
{
    if(nv->getVisitorType() == osg::NodeVisitor::CULL_VISITOR) {
	    osgUtil::CullVisitor *cv = static_cast<osgUtil::CullVisitor*>(nv);
	    if(((cv->getTraversalMask() & (~0x20)) > 0)){
		    osg::Vec3 localEyeCoords = nv->getEyePoint();
		    osg::StateSet* stateset;
		    if(cv->getCullMask() & 0x10000000){
			    stateset = _rightEyeStateset.get();
		    } else {
			    stateset = _leftEyeStateset.get();
		    }
		    osg::TexGen* transformTexgen = dynamic_cast<osg::TexGen*>(stateset->getTextureAttribute(0,osg::StateAttribute::TEXGEN));
		    transformTexgen->getPlane(osg::TexGen::R)[3] = 0.5 * M_1_PI * atan2(localEyeCoords.y(),localEyeCoords.x());
		    cv->pushStateSet(stateset);
		    // note, callback is repsonsible for scenegraph traversal so
		    // should always include call the traverse(node,nv) to ensure
		    // that the rest of cullbacks and the scene graph are traversed.
		    traverse(node,nv);

		    cv->popStateSet();
            return;
        }
    }
	// note, callback is repsonsible for scenegraph traversal so
	// should always include call the traverse(node,nv) to ensure
	// that the rest of cullbacks and the scene graph are traversed.
	traverse(node,nv);
}
bool ibrCallback_readLocalData(osg::Object&, osgDB::Input&) { return false; }

bool ibrCallback_writeLocalData(const osg::Object&, osgDB::Output&) { return true; }

REGISTER_DOTOSGWRAPPER(ibrCallback)
(
    new osg_ibr::ibrCallback,
    "ibrCallback",
	"Object NodeCallback ibrCallback",
	&ibrCallback_readLocalData,
	&ibrCallback_writeLocalData
);
REGISTER_OBJECT_WRAPPER( osg_ibr_ibrCallback, new osg_ibr::ibrCallback, osg_ibr::ibrCallback, "osg::Object osg::NodeCallback" ) {}
