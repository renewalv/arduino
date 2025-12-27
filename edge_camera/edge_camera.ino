#include <Arduino.h>
#include <dualshock-detection_inferencing.h>
#include "edge-impulse-sdk/dsp/image/image.hpp"

#include "ei_camera.h"
#include "camera_wifi.h"

/**
 * @brief      Arduino setup function
 */
void setup()
{
    /**************** Serial debug init ****************/
    Serial.begin(115200);
    Serial.setDebugOutput(true);
    delay(500);
    Serial.println();
    Serial.println("############### Edge Impulse Inferencing Begin ###############");
    
    /**************** Camera init ****************/
    if (ei_camera_init() == false)
    {
        log_e("Failed to initialize Camera!\r\n");
    }
    else
    {
        log_i("Camera initialized\r\n");
    }

#ifdef WEB_GRAPHICS
    /**************** WiFi init ****************/
    initWiFi();
    log_i("\nWiFi inited");

    startCameraServer();

    Serial.print("Use 'http://");
    Serial.print(WiFi.localIP());
    Serial.println("' to connect");
#endif

    log_i("\nStarting continious inference in 2 seconds...\n");
    ei_sleep(2000);
}

/**
 * @brief      Get data and run inferencing
 *
 * @param[in]  debug  Get debug info if true
 */
void loop()
{

    // instead of wait_ms, we'll wait on the signal, this allows threads to cancel us...
    if (ei_sleep(5) != EI_IMPULSE_OK)
    {
        return;
    }

    snapshot_buf = (uint8_t *)malloc(EI_CAMERA_RAW_FRAME_BUFFER_COLS * EI_CAMERA_RAW_FRAME_BUFFER_ROWS * EI_CAMERA_FRAME_BYTE_SIZE);

    // check if allocation was successful
    if (snapshot_buf == nullptr)
    {
        ei_printf("ERR: Failed to allocate snapshot buffer!\n");
        free(snapshot_buf);
        return;
    }

    ei::signal_t signal;
    signal.total_length = EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT;
    signal.get_data = &ei_camera_get_data;

    if (ei_camera_capture((size_t)EI_CLASSIFIER_INPUT_WIDTH, (size_t)EI_CLASSIFIER_INPUT_HEIGHT, snapshot_buf) == false)
    {
        ei_printf("Failed to capture image\r\n");
        free(snapshot_buf);
        return;
    }

    // Run the classifier
    ei_impulse_result_t result = {0};

    EI_IMPULSE_ERROR err = run_classifier(&signal, &result, debug_nn);
    if (err != EI_IMPULSE_OK)
    {
        ei_printf("ERR: Failed to run classifier (%d)\n", err);
        free(snapshot_buf);
        return;
    }

    // print the predictions
    ei_printf("Predictions (DSP: %d ms., Classification: %d ms., Anomaly: %d ms.): \n",
              result.timing.dsp, result.timing.classification, result.timing.anomaly);

#if EI_CLASSIFIER_OBJECT_DETECTION == 1
    bool bb_found = result.bounding_boxes[0].value > 0;
    for (size_t ix = 0; ix < result.bounding_boxes_count; ix++)
    {
        auto bb = result.bounding_boxes[ix];
        if (bb.value == 0)
        {
            continue;
        }
        ei_printf("    %s (%f) [ x: %u, y: %u, width: %u, height: %u ]\n", bb.label, bb.value, bb.x, bb.y, bb.width, bb.height);
    }
    if (!bb_found)
    {
        ei_printf("    No objects found\n");
    }
#else
    for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++)
    {
        ei_printf("    %s: %.5f\n", result.classification[ix].label,
                  result.classification[ix].value);
    }
#endif

#if EI_CLASSIFIER_HAS_ANOMALY == 1
    ei_printf("    anomaly score: %.3f\n", result.anomaly);
#endif

    free(snapshot_buf);
}
