file(GLOB SRC_FILES
	"*.h"
    "*.cpp"
)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    ${SRC_FILES}
)

set(FOLDERS
	sample
	middleware
	util
	osLib
	packetHandler
)

foreach(folder ${FOLDERS})
	add_subdirectory(${folder})
endforeach(folder)

