set(pri_req  common bt) 
file(GLOB_RECURSE SOURCES *.c)


# Create a sources variable with a link to all cpp files to compile
set(SOURCES
    src/*.c
)

# Set the directories that should be included in the build command for this target
# when running g++ these will be included as -I/directory/path/
set(hello_headers
    ${PROJECT_SOURCE_DIR}/include
)

idf_component_register(SRCS ${SOURCES}
                       INCLUDE_DIRS .
                       REQUIRES ${pri_req})