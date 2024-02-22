idf_component_register(SRCS "bottle-filler.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES driver nvs_flash esp_http_server settings-manager app_update pthread
                    EMBED_FILES "index.html.gz" "manifest.json" "logo.svg.gz")