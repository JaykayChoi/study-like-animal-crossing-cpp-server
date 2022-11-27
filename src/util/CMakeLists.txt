file(GLOB SRC_FILES  
	"*.h"
    "*.cpp"
)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    ${SRC_FILES}
)
