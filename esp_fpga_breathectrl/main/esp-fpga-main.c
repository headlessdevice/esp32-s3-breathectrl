#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"

#include "soc/gpio_struct.h"
#include "driver/gpio.h"
#include "qlspi_s3.h"
#include "h2d_protocol.h"

#define PERIPH_BASE                                             (0x40000000)
#define FPGA_PERIPH_BASE                                        (PERIPH_BASE + 0x00020000)    
#define FPGA_ONION_PERIPH_BASE_ADDR                             FPGA_PERIPH_BASE            //0x40020000

#define FPGA_ONION_BREATHECTRL_MODULE_OFFSET                    0x00003000

#define FPGA_ONION_BREATHECTRL_REG_OFFSET_BREATHE_0_CONFIG      0x0000
#define FPGA_ONION_BREATHECTRL_REG_OFFSET_BREATHE_1_CONFIG      0x0004
#define FPGA_ONION_BREATHECTRL_REG_OFFSET_BREATHE_2_CONFIG      0x0008

#define FPGA_ONION_BREATHECTRL_REG_ADDR_BREATHE_0_CONFIG        (uint32_t*)(FPGA_ONION_PERIPH_BASE_ADDR + \
                                                                FPGA_ONION_BREATHECTRL_MODULE_OFFSET + \
                                                                FPGA_ONION_BREATHECTRL_REG_OFFSET_BREATHE_0_CONFIG)  // (0x40020000+0x00003000+0x00000000 = 0x40023000)

#define FPGA_ONION_BREATHECTRL_REG_ADDR_BREATHE_1_CONFIG        (uint32_t*)(FPGA_ONION_PERIPH_BASE_ADDR + \
                                                                FPGA_ONION_BREATHECTRL_MODULE_OFFSET + \
                                                                FPGA_ONION_BREATHECTRL_REG_OFFSET_BREATHE_1_CONFIG)

#define FPGA_ONION_BREATHECTRL_REG_ADDR_BREATHE_2_CONFIG        (uint32_t*)(FPGA_ONION_PERIPH_BASE_ADDR + \
                                                                FPGA_ONION_BREATHECTRL_MODULE_OFFSET + \
                                                                FPGA_ONION_BREATHECTRL_REG_OFFSET_BREATHE_2_CONFIG)

extern void esp32_init_ql_spi(void);

uint32_t clock_cycles = 0;
uint32_t clock_cycles_per_step = 0; 
uint32_t clock_rate_hz = 12000000;
uint32_t io_pad_pwm_value = 0;

void breathe_enable(uint8_t pad_num, uint32_t breathe_period_ms){
    // clock_cycles/1000msec = clock_rate_hz
    clock_cycles = ((float)clock_rate_hz/1000) * breathe_period_ms; // prevent 32-bit overflow!
    // remember that the value passed in here is for the duration of the breathe cycle (inhale+exhale)
    // we need to convert this to clock_cycles/step, where total_steps = 2*(1<<PWM_RESOLUTION_BITS)
    clock_cycles_per_step = (float)clock_cycles/512;

    if (pad_num == 22){
        io_pad_pwm_value = (1 << 31) | (clock_cycles_per_step & 0xFFFFFF);
        QLSPI_Write_S3_Mem(FPGA_ONION_BREATHECTRL_REG_ADDR_BREATHE_0_CONFIG, &io_pad_pwm_value, 4);   
    }
    else if (pad_num == 21){
        io_pad_pwm_value = (1 << 31) | (clock_cycles_per_step & 0xFFFFFF);
        QLSPI_Write_S3_Mem(FPGA_ONION_BREATHECTRL_REG_ADDR_BREATHE_1_CONFIG, &io_pad_pwm_value, 4);   
    }
    else if (pad_num == 18){
        io_pad_pwm_value = (1 << 31) | (clock_cycles_per_step & 0xFFFFFF);
        QLSPI_Write_S3_Mem(FPGA_ONION_BREATHECTRL_REG_ADDR_BREATHE_2_CONFIG, &io_pad_pwm_value, 4);   
    }
}

void breathe_disable(uint8_t pad_num){
    if (pad_num == 22){
        QLSPI_Write_S3_Mem(FPGA_ONION_BREATHECTRL_REG_ADDR_BREATHE_0_CONFIG, 0, 4);   
    }
    else if (pad_num == 21){
        QLSPI_Write_S3_Mem(FPGA_ONION_BREATHECTRL_REG_ADDR_BREATHE_1_CONFIG, 0, 4);   
    }
    else if (pad_num == 18){
        QLSPI_Write_S3_Mem(FPGA_ONION_BREATHECTRL_REG_ADDR_BREATHE_2_CONFIG, 0, 4);   
    }
}

void breathe_leds(){
    while(1) {
        breathe_enable(22,1800);
        vTaskDelay(3000 / portTICK_PERIOD_MS);
        breathe_disable(22);
        vTaskDelay(5000 / portTICK_PERIOD_MS);

        breathe_enable(21,1800);
        vTaskDelay(3000 / portTICK_PERIOD_MS);
        breathe_disable(21);
        vTaskDelay(5000 / portTICK_PERIOD_MS);

        breathe_enable(18,1800);
        vTaskDelay(3000 / portTICK_PERIOD_MS);
        breathe_disable(18);
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    esp32_init_ql_spi();
    xTaskCreate(breathe_leds, "LED_BREATHE_TASK", 2048, NULL, 10, NULL);
}
