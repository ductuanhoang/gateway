set(pri_req common bt freertos) 
file(GLOB_RECURSE SOURCES *.c)
idf_component_register(SRCS ${SOURCES}
                    INCLUDE_DIRS .
                    REQUIRES ${pri_req})


