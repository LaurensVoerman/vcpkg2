#include <osg/TexGen>
#include <osgDB/Registry>
#include <osgDB/ObjectWrapper>
#include <osg/MatrixTransform>
#include <osgUtil/CullVisitor>
#include <osgViewer/Renderer>

#include <osg/StateSet>
#include <osg/Multisample>
#include "osg_ibr.h"

namespace osg_ibr {
	class IBRBB_EXPORT eyeSelectCallback :  public osg::NodeCallback
{
	class eyeSelectData : public Referenced {
	public:
		int frameNum;
		int eye;
	};
	public:
		eyeSelectCallback() {}
		eyeSelectCallback(const osg_ibr::eyeSelectCallback&,const osg::CopyOp&) {}
		META_Object(osg_ibr,eyeSelectCallback);
		virtual void operator()(osg::Node* node, osg::NodeVisitor* nv) {
            if(nv->getVisitorType() == osg::NodeVisitor::CULL_VISITOR) {
			    osgUtil::CullVisitor *cv = static_cast<osgUtil::CullVisitor *>(nv);
				bool right_eye = false;
				osg::RenderInfo &ri = cv->getRenderInfo();
//				osg::Camera *cam = ri.getCurrentCamera(); this cam is NULL
				osg::View *view = ri.getView();
				if (view) {
					osg::Camera *cam = view->getCamera();
					if (cam) {
						osgViewer::Renderer *ren = dynamic_cast<osgViewer::Renderer *>(cam->getRenderer());
						if (ren) {
							osgUtil::SceneView* sv = ren->getSceneView(0);
							if (sv) {
								osgUtil::CullVisitor *cr = sv->getCullVisitorRight();
								right_eye = (cr == nv);
							}
							//for CullThreadPerCameraDrawThreadPercontext and DrawThreadPercontext
							sv = ren->getSceneView(1);
							if (!right_eye && sv) {
								osgUtil::CullVisitor *cr = sv->getCullVisitorRight();
								right_eye = (cr == nv);
							}
						}
					}
				}
				osg::Group *grp = dynamic_cast<osg::Group *>(node);
				if (grp->getNumChildren() != 2) {
					traverse(node,nv);
				} else {
					grp->getChild(right_eye ? 1 :0 )->accept(*nv);
				}
			} else {
				traverse(node,nv);
			}
		}
};
}
bool eyeSelectCallback_readLocalData(osg::Object&, osgDB::Input&) { return false; }

bool eyeSelectCallback_writeLocalData(const osg::Object&, osgDB::Output&) { return true; }

REGISTER_DOTOSGWRAPPER(eyeSelectCallback)
(
	new osg_ibr::eyeSelectCallback,
	"eyeSelectCallback",
	"Object NodeCallback eyeSelectCallback",
	&eyeSelectCallback_readLocalData,
	&eyeSelectCallback_writeLocalData
);
REGISTER_OBJECT_WRAPPER( osg_ibr_eyeSelectCallback, new osg_ibr::eyeSelectCallback, osg_ibr::eyeSelectCallback, "osg::Object osg::NodeCallback" ) {}
