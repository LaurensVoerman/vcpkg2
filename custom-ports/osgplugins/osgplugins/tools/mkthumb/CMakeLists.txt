set(TARGET_NAME mkthumb)
set(TARGET_SRC
	mkthumb.cpp
	../../src/CustomNodes/StaticLink.cpp
)

SETUP_EXE(1)
target_link_libraries(${TARGET_NAME} ${OPENSCENEGRAPH_LIBRARIES} osg_ibr_library)
IF (NOT WIN32)
  target_link_libraries(${TARGET_NAME} GL)
ENDIF(NOT WIN32)
SET_PROPERTY(TARGET ${TARGET_NAME} PROPERTY FOLDER "Tools")

