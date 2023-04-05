set(AWS_REQUIRES  common bt) 


# Set the directories that should be included in the build command for this target
# when running g++ these will be included as -I/directory/path/
message(CMAKE_CURRENT_LIST_DIR="${CMAKE_CURRENT_LIST_DIR}")

set (AWS_SOURCES  
    ${CMAKE_CURRENT_LIST_DIR}/src
)

set(INCLUDE_AWS_DIRS
    ${CMAKE_CURRENT_LIST_DIR}/include
)

idf_component_register(SRCS ${AWS_SOURCES}
                    INCLUDE_DIRS ${INCLUDE_AWS_DIRS}
                    REQUIRES ${AWS_REQUIRES})