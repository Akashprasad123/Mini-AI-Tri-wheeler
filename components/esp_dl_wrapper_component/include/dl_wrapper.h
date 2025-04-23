#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Public Functions */

void wrapper_load_model(void);
void wrapper_run_model_test(void);
void wrapper_delete_model(void);
void wrapper_run_model_inference(float *input_data, float **output_data);
void wrapper_clean(void);

#ifdef __cplusplus
    }
#endif