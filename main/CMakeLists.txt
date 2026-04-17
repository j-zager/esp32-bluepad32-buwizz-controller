set(srcs "main.cpp" "my_platform.cpp")

set(requires 
    "bluepad32" 
    "btstack" 
    "mycontrollerex" 
    "mycontrollerconfig"
    "controllerEventManager"
    "slot_helpers"
)

idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS "."
                    REQUIRES "${requires}")
