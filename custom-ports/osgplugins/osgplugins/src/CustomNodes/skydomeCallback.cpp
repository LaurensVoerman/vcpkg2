#include <osgDB/Registry>
#include <osgDB/ObjectWrapper>
#include <osg/MatrixTransform>
#include <osgUtil/CullVisitor>
#include <osgViewer/Renderer>

namespace osg_ibr {
	class skydomeCallback :  public osg::NodeCallback
{
	public:
		skydomeCallback() {}
		skydomeCallback(const osg_ibr::skydomeCallback&,const osg::CopyOp&) {}
		META_Object(osg_ibr,skydomeCallback);
		virtual void operator()(osg::Node* node, osg::NodeVisitor* nv);
};
}

void osg_ibr::skydomeCallback::operator()(osg::Node* node, osg::NodeVisitor* nv)
{
		osg::Group *mgroup = dynamic_cast<osg::Group *>(node);
		osg::MatrixTransform *mtrans = NULL;
		if (mgroup && (mgroup->getNumChildren() != 0))
			mtrans = dynamic_cast<osg::MatrixTransform *>(mgroup->getChild(0));
		if (mtrans) {
			osg::Vec3f localEyeCoords = nv->getEyePoint();
			osg::Matrix m;
			m.makeTranslate(localEyeCoords.x(), localEyeCoords.y(), 0.0);
            if(nv->getVisitorType() == osg::NodeVisitor::CULL_VISITOR) {
			    osgUtil::CullVisitor *cv = static_cast<osgUtil::CullVisitor *>(nv);
				osg::RenderInfo &info = cv->getRenderInfo();
				const osg::Camera *cam = info.getView()->getCamera();//info.getCurrentCamera();
				if (cam) {
					const osg::Matrix &projMat = cam->getProjectionMatrix();
					double zFar = projMat(3,2) / (1.0+projMat(2,2));
					//double scale = zFar /mtrans->getChild(0)->getBound().radius();// 1/10240 --no skydome default radius  
					//m(0,0) = m(1,1) = m(2,2) = scale;
					double scale;
					if (localEyeCoords.z() > (zFar * 12.0 / 13.0)) scale = (5.0/13.0) * zFar / mtrans->getChild(0)->getBound().radius();
					else scale = sqrt(zFar*zFar-localEyeCoords.z()*localEyeCoords.z()) /mtrans->getChild(0)->getBound().radius();
					m(0,0) = m(1,1) = scale;//scale x,y to stay within clip
					m(2,2) = scale;//(localEyeCoords.z() + zFar) /mtrans->getChild(0)->getBound().radius();
				}
			}
			mtrans->setMatrix(m);
		} else {//warning: should be on a group with matrixTransform child!
		}
	// note, callback is repsonsible for scenegraph traversal so
	// should always include call the traverse(node,nv) to ensure
	// that the rest of cullbacks and the scene graph are traversed.
	traverse(node,nv);
}
bool skydomeCallback_readLocalData(osg::Object&, osgDB::Input&) { return false; }

bool skydomeCallback_writeLocalData(const osg::Object&, osgDB::Output&) { return true; }

REGISTER_DOTOSGWRAPPER(skydomeCallback)
(
	new osg_ibr::skydomeCallback,
	"skydomeCallback",
	"Object NodeCallback skydomeCallback",
	&skydomeCallback_readLocalData,
	&skydomeCallback_writeLocalData
);
REGISTER_OBJECT_WRAPPER( osg_ibr_skydomeCallback, new osg_ibr::skydomeCallback, osg_ibr::skydomeCallback, "osg::Object osg::NodeCallback" ) {}
