set(srcs "main.cpp" "my_platform.cpp")

set(requires "bluepad32" "btstack" "mycontrollerex")

idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS "."
                    REQUIRES "${requires}")
