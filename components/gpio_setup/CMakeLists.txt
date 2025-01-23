set(srcs)

idf_component_register(SRCS "i2c_config.c" "${srcs}" INCLUDE_DIRS "."
                        REQUIRES 
                            driver)