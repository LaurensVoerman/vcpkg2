##################################################### osg_ibr_library project
set(TARGET_NAME osg_ibr_library)
set(TARGET_H
	osg_ibr.h
	FadingLOD.h
	FadingPagedLOD.h
	RCBumpMapping.h
	PointStreamGeometry.h
	../Volume/GradientTransferFunction.h
	../Volume/Property.h
	../Volume/RayTracedTechnique.h
	../Volume/Volume.h
	../Volume/VolumeShaderGenerator.h
)
set(TARGET_SRC
	osg_ibr.cpp
	FadingLOD.cpp
	FadingPagedLOD.cpp
	eyeSelectCallback.cpp
	MoveEarthySkyWithEye.cpp
	skydomeCallback.cpp	
	RCBumpMapping.cpp	
	PointStreamGeometry.cpp
	../Volume/GradientTransferFunction.cpp
	../Volume/Property.cpp
	../Volume/RayTracedTechnique.cpp
	../Volume/Volume.cpp
	../Volume/VolumeShaderGenerator.cpp
	setVsyncCountCallback.cpp
)

add_library(osg_ibr_library STATIC ${TARGET_H} ${TARGET_SRC})
install(TARGETS osg_ibr_library ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
install(FILES ${TARGET_H} DESTINATION ${CMAKE_INSTALL_PREFIX}/include)

target_link_libraries(${TARGET_NAME} ${OPENSCENEGRAPH_LIBRARIES})
SET_PROPERTY(TARGET osg_ibr_library PROPERTY FOLDER "Plugins")


##################################################### osg_ibr_library project END

IF (BUILD_PLUGINS)
##################################################### osg_ibr project
set(TARGET_NAME osg_ibr)
set(TARGET_SRC
	setVsyncCountCallback.cpp
	StaticLink.cpp
)

set(DYNAMIC_OPENSCENEGRAPH 1)

add_definitions(-D_USRDLL)

SETUP_PLUGIN(${TARGET_NAME})
target_link_libraries(${TARGET_NAME} ${OPENSCENEGRAPH_LIBRARIES} osg_ibr_library)

SET_PROPERTY(TARGET ${TARGET_NAME} PROPERTY FOLDER "Plugins")


##################################################### osg_ibr project END

ENDIF (BUILD_PLUGINS)