#pragma once

#include "osg_ibr.h"
#include <osg/PagedLOD>
#include <osg/StateSet>


/////////////// adding FadingPagedLOD:
namespace osg_ibr {

    class IBRBB_EXPORT FadingPagedLOD : public osg::PagedLOD
    {
        static osg::ref_ptr<osg::StateSet> invtransparentState[16];
        static osg::ref_ptr<osg::StateSet> transparentState[16];
    public:
        FadingPagedLOD() { init(); }
        FadingPagedLOD(const osg::PagedLOD& plod, const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY): osg::PagedLOD(plod, copyop) {}
        FadingPagedLOD(const osg_ibr::FadingPagedLOD& plod, const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY): osg::PagedLOD(plod, copyop) {}
        META_Object(osg_ibr, FadingPagedLOD);
        virtual bool addChild(Node *child) { return osg::PagedLOD::addChild(child); }
        virtual bool addChild(Node *child, float min, float max) { return osg::PagedLOD::addChild(child, min, max); }
        virtual bool addChild(Node *child, float min, float max, const std::string& filename, float priorityOffset = 0.0f, float priorityScale = 1.0f) { return osg::PagedLOD::addChild(child, min, max, filename, priorityOffset, priorityScale); }
        void traverse(osg::NodeVisitor& nv);
        static void init();
        static osg::StateSet* getInvtransparentState(unsigned int i) { return invtransparentState[i].get(); }
        static osg::StateSet* getTransparentState(unsigned int i) { return transparentState[i].get(); }
		virtual bool removeExpiredChildren(double expiryTime, unsigned int expiryFrame, osg::NodeList& removedChildren);

    };
}
