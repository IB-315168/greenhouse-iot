#include "hardware_controller.h"

#include <message_buffer.h>
#include <stdio.h>

#include "co2.h"
#include "humidity_temperature.h"
#include "water_controller.h"

extern MessageBufferHandle_t upLinkMessageBufferHandle;
extern EventGroupHandle_t    xCreatedEventGroup;

void hc_toggle_handler_task(void *pvParameters);
void hc_handler_task(void *pvParameters);

void hc_handler_initialise(UBaseType_t measurement_priority, UBaseType_t toggle_priority)
{
    xTaskCreate(hc_toggle_handler_task, "Water Toggler", configMINIMAL_STACK_SIZE, NULL, toggle_priority, NULL);
    xTaskCreate(hc_handler_task, "Hardware Controller", configMINIMAL_STACK_SIZE, NULL, measurement_priority, NULL);
}

void hc_toggle_handler_task(void *pvParameters)
{
    for (;;)
    {
        xEventGroupWaitBits(xCreatedEventGroup, BIT_0, pdTRUE, pdFALSE, portMAX_DELAY);
        puts("Toggling water with event groups\n");
        if (water_controller_get_state())
        {
            water_controller_off();
        }
        else
        {
            water_controller_on();
        }
    }
}

void hc_handler_task(void *pvParameters)
{
    TickType_t       xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency    = pdMS_TO_TICKS(60000UL);
    uint8_t          counter       = 0;

    for (;;)
    {
        xTaskDelayUntil(&xLastWakeTime, xFrequency);

        co2_measure();
        hum_temp_measure();

        uint16_t temp = get_last_temperature_measurement();
        uint16_t hum  = get_last_humidity_measurement();
        uint16_t co2  = co2_get_latest_measurement();

        counter++;
        printf("Measurement [%d] {\n\tTemperature: %u\n\tHumidity: %d\n\tCO2: %u\n}\n", counter, temp, hum, co2);

        if (counter == 5)
        {
            sensor_data_t data = {temp, hum, co2};

            // TODO: Add error handling
            xMessageBufferSend(upLinkMessageBufferHandle, (void *) &data, sizeof(sensor_data_t), portMAX_DELAY);

            counter = 0;
        }
    }
}