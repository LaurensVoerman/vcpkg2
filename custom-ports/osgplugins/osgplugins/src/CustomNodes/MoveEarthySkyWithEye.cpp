#include <osg/TexGen>
#include <osgDB/Registry>
#include <osgDB/ObjectWrapper>
#include <osg/MatrixTransform>
#include <osgUtil/CullVisitor>

#include "osg_ibr.h"

namespace osg_ibr {
class IBRBB_EXPORT MoveEarthySkyWithEyePointTransform : public osg::Transform
{
	public:
		MoveEarthySkyWithEyePointTransform() {}
		MoveEarthySkyWithEyePointTransform(const osg_ibr::MoveEarthySkyWithEyePointTransform&,const osg::CopyOp&) {}
		META_Object(osg_ibr,MoveEarthySkyWithEyePointTransform);
	/** Get the transformation matrix which moves from local coords to world coords.*/
	virtual bool computeLocalToWorldMatrix(osg::Matrix& matrix,osg::NodeVisitor* nv) const 
	{
        // computebound can pass a NULLPTR for the nodevisitor
        if(nv && nv->getVisitorType() == osg::NodeVisitor::CULL_VISITOR) {
		    osgUtil::CullVisitor* cv = static_cast<osgUtil::CullVisitor*>(nv);
			osg::Vec3 eyePointLocal = cv->getEyeLocal();
//			matrix.preMult(osg::Matrix::translate(eyePointLocal.x(),eyePointLocal.y(),0.0f));
			matrix.preMult(osg::Matrix::translate(eyePointLocal));
		}
		return true;
	}

	/** Get the transformation matrix which moves from world coords to local coords.*/
	virtual bool computeWorldToLocalMatrix(osg::Matrix& matrix,osg::NodeVisitor* nv) const
	{
        // computebound can pass a NULLPTR for the nodevisitor
        if(nv && nv->getVisitorType() == osg::NodeVisitor::CULL_VISITOR) {
		    osgUtil::CullVisitor* cv = static_cast<osgUtil::CullVisitor*>(nv);
			osg::Vec3 eyePointLocal = cv->getEyeLocal();
//			matrix.postMult(osg::Matrix::translate(-eyePointLocal.x(),-eyePointLocal.y(),0.0f));
			matrix.postMult(osg::Matrix::translate(-eyePointLocal));
		}
		return true;
	}
};
}
bool MoveEarthySkyWithEyePointTransform_readLocalData(osg::Object&, osgDB::Input&) { return false; }

bool MoveEarthySkyWithEyePointTransform_writeLocalData(const osg::Object&, osgDB::Output&) { return true; }

REGISTER_DOTOSGWRAPPER(MoveEarthySkyWithEyePointTransform)
(
	new osg_ibr::MoveEarthySkyWithEyePointTransform,
	"MoveEarthySkyWithEyePointTransform",
	"Object Node Transform Group MoveEarthySkyWithEyePointTransform",
	&MoveEarthySkyWithEyePointTransform_readLocalData,
	&MoveEarthySkyWithEyePointTransform_writeLocalData
);
REGISTER_OBJECT_WRAPPER( osg_ibr_MoveEarthySkyWithEyePointTransform, new osg_ibr::MoveEarthySkyWithEyePointTransform, osg_ibr::MoveEarthySkyWithEyePointTransform, "osg::Object osg::Node osg::Group osg::Transform" ) {}
