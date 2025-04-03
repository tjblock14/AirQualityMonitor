set(srcs 
        "sensirion_gas_index_algorithm.c" )

idf_component_register(SRCS "${srcs}" INCLUDE_DIRS ".")