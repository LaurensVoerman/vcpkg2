set(TARGET_NAME osgconvEx)
set(TARGET_SRC
	osgconvEx.cpp
	OrientationConverter.cpp
)
set(TARGET_H
	OrientationConverter.h
)

SETUP_EXE(1)
target_link_libraries(${TARGET_NAME} ${OPENSCENEGRAPH_LIBRARIES})

SET_PROPERTY(TARGET ${TARGET_NAME} PROPERTY FOLDER "Tools")

