#include "dl_wrapper.h"
#include "dl_model_base.hpp"

#define TAG "DL_WRAPPER"

// extern const uint8_t model_espdl[] asm("_binary_model_v3_0_espdl_start");
static dl::Model *model = nullptr;
static std::map<std::string, dl::TensorBase *> model_inputs, model_outputs;
static dl::TensorBase *model_input = nullptr, *model_output = nullptr;
static dl::TensorBase *input_tensor = nullptr, *output_tensor = nullptr;

static void wrapper_input_to_tensor(float *input_data){
    input_tensor = new dl::TensorBase(model_input->get_shape(), input_data, 0, dl::DATA_TYPE_FLOAT);
    model_input->assign(input_tensor);
}

static void wrapper_get_model_inference_output(float **output_data){
    output_tensor = new dl::TensorBase(model_output->get_shape(), nullptr, 0, dl::DATA_TYPE_FLOAT);
    output_tensor->assign(model_output);
    *output_data = (float *)output_tensor->get_element_ptr();
}

void wrapper_clean(){
    if (input_tensor != nullptr) {
        delete input_tensor;
        input_tensor = nullptr;
    }
    if (output_tensor != nullptr) {
        delete output_tensor;
        output_tensor = nullptr;
    }
    else {
        ESP_LOGE(TAG, "Tensor already deleted or not loaded.");
    }
}

void wrapper_load_model(void){
    // model = new dl::Model((const char *)model_espdl, fbs::MODEL_LOCATION_IN_FLASH_RODATA);
    model = new dl::Model("model", fbs::MODEL_LOCATION_IN_FLASH_PARTITION, 0);
    // Keep parameter in FLASH, saves PSRAM/internal RAM, lower performance.
    // dl::Model *model = new dl::Model((const char *)model_espdl, fbs::MODEL_LOCATION_IN_FLASH_RODATA, 0,
    // dl::MEMORY_MANAGER_GREEDY, nullptr, false);
    model_inputs = model->get_inputs();
    model_input = model_inputs.begin()->second;
    model_outputs = model->get_outputs();
    model_output = model_outputs.begin()->second;
}

void wrapper_run_model_test(void){
    if (model == nullptr) {
        ESP_LOGE(TAG, "Model not loaded. Please load the model first.");
        return;
    }
    ESP_ERROR_CHECK(model->test());
    model->profile();
}

void wrapper_run_model_inference(float *input_data, float **output_data){
    wrapper_input_to_tensor(input_data);
    model->run();
    wrapper_get_model_inference_output(output_data);
    // wrapper_delete_tensor();
}

void wrapper_delete_model(void){
    if (model != nullptr) {
        delete model;
        model = nullptr;
        model_input = nullptr;
        model_output = nullptr;
        model_inputs.clear();
        model_outputs.clear();
    } else {
        ESP_LOGE(TAG, "Model already deleted or not loaded.");
    }
}