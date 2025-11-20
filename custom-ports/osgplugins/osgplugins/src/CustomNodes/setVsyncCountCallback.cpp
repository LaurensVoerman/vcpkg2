#include <osg/TexGen>
#include <osgDB/Registry>
#include <osgDB/ObjectWrapper>
#include <osg/MatrixTransform>
#include <osgUtil/CullVisitor>
#include <osgViewer/Renderer>
#include <osgViewer/api/Win32/GraphicsWindowWin32>
#include <osg/StateSet>
#include <osg/Multisample>

#include "osg_ibr.h"
#if USE_NV_SWAPGROUP
#ifndef WGL_NV_swap_group
#define WGL_NV_swap_group 1
typedef BOOL(WINAPI* PFNWGLJOINSWAPGROUPNVPROC) (HDC hDC, GLuint group);
typedef BOOL(WINAPI* PFNWGLBINDSWAPBARRIERNVPROC) (GLuint group, GLuint barrier);
typedef BOOL(WINAPI* PFNWGLQUERYSWAPGROUPNVPROC) (HDC hDC, GLuint* group, GLuint* barrier);
typedef BOOL(WINAPI* PFNWGLQUERYMAXSWAPGROUPSNVPROC) (HDC hDC, GLuint* maxGroups, GLuint* maxBarriers);
typedef BOOL(WINAPI* PFNWGLQUERYFRAMECOUNTNVPROC) (HDC hDC, GLuint* count);
typedef BOOL(WINAPI* PFNWGLRESETFRAMECOUNTNVPROC) (HDC hDC);
#ifdef WGL_WGLEXT_PROTOTYPES
BOOL WINAPI wglJoinSwapGroupNV(HDC hDC, GLuint group);
BOOL WINAPI wglBindSwapBarrierNV(GLuint group, GLuint barrier);
BOOL WINAPI wglQuerySwapGroupNV(HDC hDC, GLuint* group, GLuint* barrier);
BOOL WINAPI wglQueryMaxSwapGroupsNV(HDC hDC, GLuint* maxGroups, GLuint* maxBarriers);
BOOL WINAPI wglQueryFrameCountNV(HDC hDC, GLuint* count);
BOOL WINAPI wglResetFrameCountNV(HDC hDC);
#endif
#endif /* WGL_NV_swap_group */

/*
// todo: look at 
   BOOL wglDelayBeforeSwapNV(HDC hDC, GLfloat seconds)
   https://registry.khronos.org/OpenGL/extensions/NV/WGL_NV_delay_before_swap.txt
   ---
   https://registry.khronos.org/OpenGL/extensions/NV/WGL_NV_swap_group.txt
   ---
   https://registry.khronos.org/OpenGL/extensions/EXT/WGL_EXT_swap_control_tear.txt
   //set negative swapinterval
   ---

*/
//get the framecounter value for use by shader
#else


#include <dwmapi.h>
#pragma comment(lib,"Dwmapi.lib")//for __imp_DwmGetCompositionTimingInfo
typedef LONG NTSTATUS;
#define STATUS_SUCCESS                   ((NTSTATUS)0x00000000L) 
#include <d3dkmthk.h>

#endif
namespace osg_ibr {

		setVsyncCountCallback::setVsyncCountCallback() {}
		setVsyncCountCallback::setVsyncCountCallback(const setVsyncCountCallback& org, const osg::CopyOp& copyop) {}
		void setVsyncCountCallback::drawImplementation
		(osg::RenderInfo& ri, const osg::Drawable* drawable) const
		{

			const osg::StateSet* stateset = drawable->getStateSet();
			const osg::Uniform* c = stateset->getUniform("VsyncCount");
			osg::Uniform* d = const_cast<osg::Uniform*>(c);
			unsigned int contextID = ri.getState()->getContextID();
			osg::GraphicsContext *gc = ri.getState()->getGraphicsContext();
			osgViewer::GraphicsWindowWin32 *win = dynamic_cast<osgViewer::GraphicsWindowWin32*>(gc);
#if 1
			const osg::Uniform* t = stateset->getUniform("compositeTexture");
			int skip = 0;
			if (t) t->get(skip);
			if (skip == 1) {
				osg::Uniform* u = const_cast<osg::Uniform*>(t);
				u->set(0);
#ifndef WGL_EXT_swap_control
#define WGL_EXT_swap_control 1
				typedef BOOL(WINAPI* PFNWGLSWAPINTERVALEXTPROC) (int interval);
				typedef int (WINAPI* PFNWGLGETSWAPINTERVALEXTPROC) (void);
#endif
				PFNWGLSWAPINTERVALEXTPROC wglSwapInterval = NULL;
				PFNWGLGETSWAPINTERVALEXTPROC wglGetSwapInterval = NULL;
				wglSwapInterval = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
				wglGetSwapInterval = (PFNWGLGETSWAPINTERVALEXTPROC)wglGetProcAddress("wglGetSwapIntervalEXT");
				if (wglGetSwapInterval) {
					int oldswap = wglGetSwapInterval();
					if (wglSwapInterval) wglSwapInterval(1);
					SwapBuffers(win->getHDC());
					if (wglSwapInterval) wglSwapInterval(oldswap);
				}
			}
#endif
#if USE_NV_SWAPGROUP
			HDC dc = win->getHDC();
			if (true | osg::isGLExtensionSupported(contextID, "WGL_NV_swap_group")) {


				PFNWGLQUERYFRAMECOUNTNVPROC wglQueryFrameCountNV = (PFNWGLQUERYFRAMECOUNTNVPROC)wglGetProcAddress("wglQueryFrameCountNV");
				GLuint count = 0;
				if (wglQueryFrameCountNV != NULL) {
					wglQueryFrameCountNV(dc, &count);
				} else {
					count = 10 * ri.getState()->getFrameStamp()->getFrameNumber();
				}
				d->set((int)count);
			}
#else
			HWND hwnd =win->getHWND();
			DWM_TIMING_INFO TimingInfo;
			TimingInfo.cbSize = sizeof(DWM_TIMING_INFO);

			HRESULT result = DwmGetCompositionTimingInfo(NULL,&TimingInfo);
			int count = 12;
			if (result == S_OK) {
				
				/* results in 14 or 15
				QPC_TIME lastblank = TimingInfo.qpcVBlank; //The query performance counter value before the vertical blank.
				LARGE_INTEGER PerformanceCount;
				QueryPerformanceCounter(&PerformanceCount);
				QPC_TIME delta = lastblank - PerformanceCount.QuadPart;
				QueryPerformanceFrequency(&PerformanceCount);
				int milliSec = delta * 1000 / PerformanceCount.QuadPart;
				count = milliSec;
				*/
				//DWM_FRAME_COUNT cRefresh
				double time = ri.getState()->getFrameStamp()->getReferenceTime();
				count = (int)(TimingInfo.cRefresh/time);//164?
				//cFrameDisplayed = 0 
				count = (int)(TimingInfo.cRefresh) - prev;
				prev = (int)(TimingInfo.cRefresh);


			}
			d->set(count);
			D3DKMT_ENUMADAPTERS d3dkmt_enumadapters;
			NTSTATUS result2 = D3DKMTEnumAdapters(&d3dkmt_enumadapters);
			if (result2 == STATUS_SUCCESS) {
				if (d3dkmt_enumadapters.Adapters > 0) {
					
					D3DKMT_GETSCANLINE d3dGs;
					d3dGs.hAdapter = d3dkmt_enumadapters.Adapters[0].hAdapter;
					D3DKMDT_VIDEO_PRESENT_SOURCE d3dkmdt_video_present_source;
					d3dGs.VidPnSourceId = 0;
					NTSTATUS result3 = D3DKMTGetScanLine(&d3dGs);
					if (result2 == STATUS_SUCCESS) {
						count = d3dGs.ScanLine;
					}
				}
			}
			d->set(count);
#endif
			drawable->drawImplementation(ri);
		}

}//namespace osg_ibr
bool setVsyncCountCallback_readLocalData(osg::Object&, osgDB::Input&) { return false; }

bool setVsyncCountCallback_writeLocalData(const osg::Object&, osgDB::Output&) { return true; }

REGISTER_DOTOSGWRAPPER(setVsyncCountCallback)
	(
		new osg_ibr::setVsyncCountCallback,
		"osg_ibr::setVsyncCountCallback",
		"Object setVsyncCountCallback",
		&setVsyncCountCallback_readLocalData,
		&setVsyncCountCallback_writeLocalData
		);
REGISTER_OBJECT_WRAPPER(osg_ibr_setVsyncCountCallback, new osg_ibr::setVsyncCountCallback, osg_ibr::setVsyncCountCallback, "osg::Object") {}
