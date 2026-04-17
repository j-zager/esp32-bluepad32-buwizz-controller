idf_component_register(
    SRCS "src/MyControllerConfig.cpp" "src/slot_helpers.cpp"
    INCLUDE_DIRS "include"
    REQUIRES mycontrollerex bluepad32 controllerEventManager
)
