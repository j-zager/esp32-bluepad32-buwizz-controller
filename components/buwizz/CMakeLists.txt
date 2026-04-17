# idf_component_register(
#     SRCS "src/buwizz.cpp"
#     INCLUDE_DIRS "include"
#      REQUIRES "btstack"
#     # PRIV_REQUIRES btstack
# )


# set(srcs
#          "src/buwizz.cpp"
#          )

# # set(requires 
# #             "btstack" 
# #             "bt"
# #             "bt\host\bluedroid\api\include\api"
# #             )

# idf_component_register(SRCS "${srcs}"
#                     INCLUDE_DIRS "include"
#                     # REQUIRES ${requires}
#                     REQUIRES nvs_flash btstack app_update esp_timer
#                     )


idf_component_register(
    SRCS "src/buwizz.cpp"
    INCLUDE_DIRS "include"
    REQUIRES  bt  btstack bluepad32
)
