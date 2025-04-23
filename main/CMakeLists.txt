# idf_build_get_property(component_targets __COMPONENT_TARGETS)
# if ("___idf_espressif__esp-dl" IN_LIST component_targets)
# idf_component_get_property(espdl_dir espressif__esp-dl COMPONENT_DIR)
# elseif("___idf_esp-dl" IN_LIST component_targets)
# idf_component_get_property(espdl_dir esp-dl COMPONENT_DIR)
# endif()
# set(cmake_dir ${espdl_dir}/fbs_loader/cmake)
# include(${cmake_dir}/utilities.cmake)
# set(embed_files model/model_v3_0.espdl)

file(GLOB_RECURSE srcs "main.c" "src/*.c")

idf_component_register(SRCS "${srcs}"
                       INCLUDE_DIRS "./include")

set(image_file "${COMPONENT_DIR}/model/model_v3_0.espdl")
# esptool_py_flash_to_partition(${model_name} ${MODEL_ABS_PATH})
                       
esptool_py_flash_to_partition(flash "model" "${image_file}")

# target_add_aligned_binary_data(${COMPONENT_LIB} ${embed_files} BINARY)
