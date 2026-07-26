# set(srcs 
#     "main.cpp" 
#     "my_platform.cpp"
# )

# set(requires 
#     "bluepad32" 
#     "btstack" 
#     "mycontrollerex" 
#     "mycontrollerconfig"
#     "controllerEventManager"
#     "slot_helpers"
#     "buwizz"
# )

# idf_component_register(SRCS "${srcs}"
#                     INCLUDE_DIRS "."
#                     REQUIRES "${requires}")


idf_component_register(
    SRCS "main.cpp" "my_platform.cpp"
    INCLUDE_DIRS "."
    REQUIRES 
        bluepad32 
        btstack 
        mycontrollerex 
        mycontrollerconfig
        controllerEventManager
        slot_helpers
        buwizz
        lightmanager
)
