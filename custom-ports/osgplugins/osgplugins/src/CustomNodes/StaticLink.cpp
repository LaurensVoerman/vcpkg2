#include "osg_ibr.h"
#include "RCBumpMapping.h"
#include <osgDB/Registry>

USE_DOTOSGWRAPPER(FadingLOD)
USE_DOTOSGWRAPPER(FadingPagedLOD)
USE_DOTOSGWRAPPER(skydomeCallback)
USE_DOTOSGWRAPPER(ibrCallback)
USE_DOTOSGWRAPPER(MoveEarthySkyWithEyePointTransform)
USE_DOTOSGWRAPPER(eyeSelectCallback)
USE_DOTOSGWRAPPER(TransferFunction1D_Proxy)
USE_SERIALIZER_WRAPPER(PointStreamGeometry)

USE_SERIALIZER_WRAPPER(osg_ibr_GradientTransferFunction)
USE_DOTOSGWRAPPER(setVsyncCountCallback)