file(GLOB SRC_FILES
	"*.h"
    "*.cpp"
)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    ${SRC_FILES}
)

get_target_property(ALL_FILES ${CMAKE_PROJECT_NAME} SOURCES)
foreach(FILE ${ALL_FILES})
	file(RELATIVE_PATH RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" ${FILE})
	set(ALL_FILES_AS_LINES "${ALL_FILES_AS_LINES}${RELATIVE}\n")
endforeach()
file(WRITE AllFiles.txt "${ALL_FILES_AS_LINES}")

set(FOLDERS
	auth
	town
	dev
)

foreach(folder ${FOLDERS})
	add_subdirectory(${folder})
endforeach(folder)
