///////////////////////////////////////////////////////////////////////////////////////////////////
#include <osg/TexGen>
#include <osgDB/Registry>
#include <osgDB/ObjectWrapper>
#include <osg/MatrixTransform>
#include <osgUtil/CullVisitor>
#include <osgViewer/Renderer>


#include <osg/Multisample>
#include "NodeMaskUtils.h"
#include "FadingPagedLOD.h"

osg::ref_ptr<osg::StateSet> osg_ibr::FadingPagedLOD::transparentState[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
osg::ref_ptr<osg::StateSet> osg_ibr::FadingPagedLOD::invtransparentState[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

void osg_ibr::FadingPagedLOD::init(){
	if (!invtransparentState[15].valid()) {
		for(unsigned int i = 0; i < 16;++i) {
			transparentState[i] = new osg::StateSet();
			osg::Multisample* ms = new osg::Multisample();
			ms->setCoverage((float)(i+1)/32.0f);// 1/2 t/m 31/32
			transparentState[i]->setAttributeAndModes(ms, osg::StateAttribute::OVERRIDE|osg::StateAttribute::ON);
			transparentState[i]->setMode(GL_SAMPLE_COVERAGE_ARB, osg::StateAttribute::OVERRIDE|osg::StateAttribute::ON);
			invtransparentState[i] = new osg::StateSet();
			osg::Multisample* msI = new osg::Multisample();
			msI->setCoverage((float)(i+1)/32.0f);
			msI->setInvert(true);
			invtransparentState[i]->setAttributeAndModes(msI, osg::StateAttribute::OVERRIDE|osg::StateAttribute::ON);
			invtransparentState[i]->setMode(GL_SAMPLE_COVERAGE_ARB, osg::StateAttribute::OVERRIDE|osg::StateAttribute::ON);
		}
	}
}

////////////////////////////////// replace PagedLOD with LOD
void osg_ibr::FadingPagedLOD::traverse(osg::NodeVisitor& nv)
{
	if ((nv.getVisitorType() != osg::NodeVisitor::CULL_VISITOR) ||
		(nv.getTraversalMode() != osg::NodeVisitor::TRAVERSE_ACTIVE_CHILDREN)) { //not the normal rendering cull 
//         nv.getTraversalMask() == ~osgRC::NodeMaskUtils::DISABLE_SHADOWS)) { // behave as normal lod for shadow visitor
			PagedLOD::traverse(nv);											//behave as a normal pagedLOD
			return;
	}
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

	for(unsigned int i=0;i<_rangeList.size();++i)
	{
		if (_rangeList[i].first<=required_range && required_range<_rangeList[i].second)
		{ //child i is the one in range
			if (i<_children.size()) { //and child i is paged in

				unsigned int frameNumber = nv.getFrameStamp()?nv.getFrameStamp()->getFrameNumber():0;
				if ((i > 0)/* &&(frameNumber > 16)*/) {
					volatile unsigned int step = _perRangeDataList[i]._frameNumber;//grab a COPY as this variable could be written by an other thread
					volatile unsigned int fOLT = _frameNumberOfLastTraversal;      //grab a COPY as this variable could be written by an other thread
					double time = nv.getFrameStamp()->getReferenceTime();
					if (step > 0) {//not busy interpolating
						if (fOLT > step + 10) {// did not render this level last 10 frames
							if ((i + 1 >= _rangeList.size()) || fOLT < _perRangeDataList[i + 1]._frameNumber + 2) { //HIGHER level of detail did not render in last 2 frames
								step = fOLT + 1; // consider new
							}
						}
					}
					if (step > fOLT) {
						_perRangeDataList[i]._frameNumber = 0; //was just set by dbPager - USING 0 AS MARKER "Busy interpolating"
//count down time (1 sec)
#define FADE_TIME 1.0
						_perRangeDataList[i]._timeStamp = time + FADE_TIME; //future time point (fully faded in)
					}
					double dstep = (_perRangeDataList[i]._timeStamp - time) / FADE_TIME;
					step = 32.0 * (1.0 - dstep);
					if (step < 32) {
						//if just paged in: blend in
						unsigned int mainChild, secondChild;
						if (step < 16) {
							mainChild = i - 1;
							secondChild = i;
						} else {
							mainChild = i;
							secondChild = i - 1;
							step = 31 - step;
						}
						if (nv.getTraversalMask() == osgRC::NodeMaskUtils::SHADOWS) step = 0; //render MAIN child for shadows - it's the best guess (not great)
						if (step != 0) {
							cv->pushStateSet(invtransparentState[step].get());
							_children[mainChild]->accept(nv);
							cv->popStateSet();
							cv->pushStateSet(transparentState[step].get());
							_children[secondChild]->accept(nv);
							cv->popStateSet();
						} else {
							_children[mainChild]->accept(nv);
						}
//						_perRangeDataList[i]._frameNumber++;
						//DONT UPDATE TIMESTAMP
//						_perRangeDataList[i]._timeStamp=nv.getFrameStamp()?nv.getFrameStamp()->getReferenceTime():0.0; 
						_frameNumberOfLastTraversal = frameNumber;
						return;
					}
				}
				// i == 0 or step > 32
				_children[i]->accept(nv);
				_frameNumberOfLastTraversal = frameNumber;
				_perRangeDataList[i]._frameNumber = frameNumber;
				_perRangeDataList[i]._timeStamp = nv.getFrameStamp() ? nv.getFrameStamp()->getReferenceTime() : 0.0;
				return;
			}
		}
	}

	//child in range is not loaded; go load it
	PagedLOD::traverse(nv);
}
bool osg_ibr::FadingPagedLOD::removeExpiredChildren(double expiryTime, unsigned int expiryFrame, osg::NodeList& removedChildren)
{
	if (_children.size()>_numChildrenThatCannotBeExpired)
	{
		unsigned cindex = _children.size() - 1;
		if (!_perRangeDataList[cindex]._filename.empty() && _perRangeDataList[cindex]._frameNumber > 32 && // added  _perRangeDataList[cindex]._frameNumber > 32
			_perRangeDataList[cindex]._timeStamp + _perRangeDataList[cindex]._minExpiryTime < expiryTime &&
			_perRangeDataList[cindex]._frameNumber + _perRangeDataList[cindex]._minExpiryFrames < expiryFrame)
		{
			osg::Node* nodeToRemove = _children[cindex].get();
			removedChildren.push_back(nodeToRemove);
			return Group::removeChildren(cindex, 1);
		}
	}
	return false;
}

bool FadingPagedLOD_readLocalData(osg::Object&, osgDB::Input&) { return false; }

bool FadingPagedLOD_writeLocalData(const osg::Object&, osgDB::Output&) { return true; }

REGISTER_DOTOSGWRAPPER(FadingPagedLOD)
(
	new osg_ibr::FadingPagedLOD,
	"FadingPagedLOD",
	"Object Node LOD PagedLOD",
	&FadingPagedLOD_readLocalData,
	&FadingPagedLOD_writeLocalData
);

REGISTER_OBJECT_WRAPPER2( osg_ibr_FadingPagedLOD, new osg_ibr::FadingPagedLOD, osg_ibr::FadingPagedLOD, "osg_ibr::FadingPagedLOD", "osg::Object osg::Node osg::LOD osg::PagedLOD" ) {}
