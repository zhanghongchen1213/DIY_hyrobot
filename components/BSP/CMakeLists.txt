set(src_dirs
            IMU
            Servo
            BSP_Drivers
            STUDIO
            Motor
            UART
            Wifi
            )

set(include_dirs
            IMU
            Servo
            BSP_Drivers
            STUDIO
            Motor
            UART
            Wifi
            )

set(requires
            nvs_flash
            driver
            spiffs
            fatfs
            bt
            esp_timer
            espressif__servo
            espressif__esp_codec_dev
            chmorgan__esp-audio-player
            chmorgan__esp-file-iterator
            espressif__esp-dsp
            )

idf_component_register(SRC_DIRS ${src_dirs} INCLUDE_DIRS ${include_dirs} REQUIRES ${requires})

component_compile_options(-ffast-math -O3 -Wno-error=format=-Wno-format)