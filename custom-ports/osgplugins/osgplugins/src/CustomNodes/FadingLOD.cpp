///////////////////////////////////////////////////////////////////////////////////////////////////
#include <osg/TexGen>
#include <osgDB/Registry>
#include <osgDB/ObjectWrapper>
#include <osg/MatrixTransform>
#include <osgUtil/CullVisitor>
#include <osgViewer/Renderer>


#include <osg/Multisample>
#include "NodeMaskUtils.h"
#include "FadingLOD.h"

////////////////////////////////// replace PagedLOD with LOD
void osg_ibr::FadingLOD::traverse(osg::NodeVisitor& nv)
{
	if ((nv.getVisitorType()!=osg::NodeVisitor::CULL_VISITOR) ||			//not a cull
        (nv.getTraversalMode() != osg::NodeVisitor::TRAVERSE_ACTIVE_CHILDREN ||  //not the normal rendering cull 
         nv.getTraversalMask() == ~osgRC::NodeMaskUtils::DISABLE_SHADOWS)) { // behave as normal lod for shadow visitor
			LOD::traverse(nv);											//behave as a normal LOD
			return;
	}
    float blendRangeFraction = 0.2f;
	osgUtil::CullVisitor *cv = static_cast<osgUtil::CullVisitor *>(&nv);
	float required_range = 0;
	if (_rangeMode==DISTANCE_FROM_EYE_POINT)
	{
		required_range = nv.getDistanceToViewPoint(getCenter(),true);
	}
	else
	{
        if(cv->getLODScale()>0.0f)
		{
            required_range = cv->clampedPixelSize(getBound()) / cv->getLODScale();
		}
		else
		{
			// fallback to selecting the highest res tile by
			// finding out the max range
			for(unsigned int i=0;i<_rangeList.size();++i)
			{
				required_range = osg::maximum(required_range,_rangeList[i].first);
			}
		}
	}
	float fadeStep = 0.0f;
	unsigned int oldChild = 0;
	unsigned int i = 0;//active child
	for(i=0;i<_rangeList.size();++i)
	{
		if (_rangeList[i].first<=required_range && required_range<_rangeList[i].second)
		{ //child i is the one in range
			{ //even if child i does not exist
				float pos = (required_range - _rangeList[i].first );
				//last child might have a huge range
				pos /= (_rangeList[i].second - _rangeList[i].first);
				oldChild = i;
				if (pos < blendRangeFraction) {
					fadeStep = 1.0f - ((1.0f / blendRangeFraction) * pos);
					for(unsigned int j=0;j<_rangeList.size();++j)
						if (_rangeList[i].first == _rangeList[j].second) oldChild = j;//find child at same limit
				}
				if (pos > (1.0f - blendRangeFraction)) {
					fadeStep = 1.0f - ((1.0f / blendRangeFraction) * (1.0f - pos));
					for(unsigned int j=0;j<_rangeList.size();++j)
						if (_rangeList[j].first == _rangeList[i].second) oldChild = j;
				}
				if (oldChild == i) fadeStep = 0.0f; //do not fade... no other child available
				else {
					float myRangeLength = _rangeList[i].second - _rangeList[i].first;
					float otherRangeLength = _rangeList[oldChild].second - _rangeList[oldChild].first;
					if (otherRangeLength < myRangeLength) {
						fadeStep = 0.0f;
						if (pos < blendRangeFraction) {
							pos = (required_range - _rangeList[i].first ) / otherRangeLength;
							if (pos < blendRangeFraction) fadeStep = 1.0f - ((1.0f / blendRangeFraction) * pos);
						} else {
							if (pos > (1.0f - blendRangeFraction)) {
								pos = (_rangeList[i].second - required_range ) / otherRangeLength;
								if (pos < blendRangeFraction) fadeStep = 1.0f - ((1.0f / blendRangeFraction) * pos);
							}
						}
					}
				}
			}
			break;
		}
	}// fadeStep range 0.0f (do not fade) up to 1.0f (50%)
	fadeStep *= 32;//range [0-16>
	unsigned int step = (unsigned int)fadeStep;//values 0 t/m 31
	if (step) {
		step >>= 1;//values 0 t/m 15
		if (step >15) {
			step = 15;//guard
		}
		if (i < _children.size() ) {
			cv->pushStateSet(FadingPagedLOD::getInvtransparentState(step));
			LOD::traverse(nv);
			cv->popStateSet();
		}
		if (oldChild < _children.size() ) {
			cv->pushStateSet(FadingPagedLOD::getTransparentState(step));
			_children[oldChild]->accept(nv);
			cv->popStateSet();
		}
	} else {
		LOD::traverse(nv);
	}

}

bool osg_ibr::FadingLOD::removeChildren(unsigned int pos,unsigned int numChildrenToRemove) {
	if ((pos) && (pos + numChildrenToRemove == _children.size())) {
		if (pos+1<_rangeList.size()) _rangeList.erase(_rangeList.begin()+pos+1, osg::minimum(_rangeList.begin()+(pos+1+numChildrenToRemove), _rangeList.end()) );
	    return Group::removeChildren(pos,numChildrenToRemove);
	}
	return osg::LOD::removeChildren(pos, numChildrenToRemove);
}

bool FadingLOD_readLocalData(osg::Object&, osgDB::Input&) { return false; }

bool FadingLOD_writeLocalData(const osg::Object&, osgDB::Output&) { return true; }

REGISTER_DOTOSGWRAPPER(FadingLOD)
(
	new osg_ibr::FadingLOD,
	"FadingLOD",
	"Object Node LOD Group",
	&FadingLOD_readLocalData,
	&FadingLOD_writeLocalData
);
REGISTER_OBJECT_WRAPPER2( osg_ibr_FadingLOD, new osg_ibr::FadingLOD, osg_ibr::FadingLOD, "osg_ibr::FadingLOD", "osg::Object osg::Node osg::Group osg::LOD" ) {}
