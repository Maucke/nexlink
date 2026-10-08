idf_component_register(SRCS "nexlink_tx.c" "nexlink_tasks.c" "nexlink_app.c" "usb_descriptors.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES esp_timer)

idf_build_set_property(MINIMAL_BUILD ON)