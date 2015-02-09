IF (NOT DEFINED OTF_FOUND)
    SET (OTF_FOUND FALSE)
ENDIF()

FIND_PATH(OTF_INCLUDE_DIRS otf.h
    $ENV{HOME}/opt/include
    $ENV{HOME}/opt/include/otf
    $ENV{HOME}/opt/include/open-trace-format
    /usr/include
)

FIND_LIBRARY(OTF_LIBRARIES otf
    $ENV{HOME}/opt/lib
    /usr/lib
)

IF (NOT OTF_LIBRARIES)
    FIND_LIBRARY(OTF_LIBRARIES open-trace-format
	$ENV{HOME}/opt/lib
	/usr/lib
    )
ENDIF()

IF (OTF_INCLUDE_DIRS AND OTF_LIBRARIES)
    SET(OTF_FOUND TRUE)
    IF (CMAKE_VERBOSE_MAKEFILE)
	MESSAGE("Using OTF_INCLUDE_DIRS = " ${OTF_INCLUDE_DIRS})
	MESSAGE("Using OTF_LIBRARIES = " ${OTF_LIBRARIES})
    ENDIF()
ENDIF()
