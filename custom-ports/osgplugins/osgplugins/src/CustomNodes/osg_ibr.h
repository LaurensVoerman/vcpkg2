#ifndef OSG_IBR_H
#define OSG_IBR_H 1

#if defined(WIN32) && defined(osg_ibr_EXPORTS)
#define IBRBB_EXPORT  __declspec(dllexport)
#else
#define IBRBB_EXPORT 
#endif

#include <osg/Object>
#include <osg/Drawable>

namespace osg_ibr {
	class IBRBB_EXPORT setVsyncCountCallback :
		public osg::Drawable::DrawCallback
	{
	public:
		setVsyncCountCallback();
		setVsyncCountCallback(const setVsyncCountCallback& org, const osg::CopyOp& copyop);

		META_Object(osg_ibr, setVsyncCountCallback);
		virtual void drawImplementation(osg::RenderInfo& ri, const osg::Drawable* drawable) const;
		mutable int prev = 0;
	};
}//namespace osg_ibr
#endif