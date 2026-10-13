idf_component_register(SRCS SRC_DIRS 
                        "."
                        "app_blue_control"
                        "app_audio_control"
                        "app_move_control"
                        "app_uart_control"
                        "all_debug"

                    INCLUDE_DIRS
                        "."
                        "app_blue_control"
                        "app_audio_control"
                        "app_move_control"
                        "app_uart_control"
                        "all_debug"
                        )        

spiffs_create_partition_image(storage ../spiffs FLASH_IN_PROJECT)   
