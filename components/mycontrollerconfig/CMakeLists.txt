idf_component_register(
    SRCS "src/MyControllerConfig.cpp" 
    INCLUDE_DIRS "include"
    REQUIRES mycontrollerex bluepad32 controllerEventManager slot_helpers
)
