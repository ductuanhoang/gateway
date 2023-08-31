set(pri_req common bt freertos esp_adc_cal) 
file(GLOB_RECURSE SOURCES *.c)
idf_component_register(SRCS ${SOURCES}
                    INCLUDE_DIRS .
                    REQUIRES ${pri_req})


