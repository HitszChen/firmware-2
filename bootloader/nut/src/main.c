/* Copyright (c) 2015-2016 Ivan Grokhotkov. All rights reserved.
 * This file is part of eboot bootloader.
 *
 * Redistribution and use is permitted according to the conditions of the
 * 3-clause BSD license to be found in the LICENSE file.
 */

#include "hw_config.h"
#include "boot_mode.h"
#include "flash.h"
#include "esp8266/esp8266_config.h"
#include "params_hal.h"
#include "ui_hal.h"
#include "core_hal.h"
#include "boot_debug.h"


#define BOOTLOADER_VERSION  1

uint8_t RESERVE_MODE=0;
uint8_t SERIAL_COM_MODE = 0;
uint8_t DEFAULT_FIRMWARE_MODE=0;
uint8_t FACTORY_RESET_MODE=0;
uint8_t NC_MODE=0;
uint8_t ALL_RESET_MODE=0;
uint8_t START_APP_MODE=0;
uint8_t OTA_FIRMWARE_MODE=0;

uint32_t BUTTON_press_time=0;

void Start_App(void)
{
    ETS_FRC1_INTR_DISABLE();
    if (!load_app(APP_START_OFFSET)){
        SWRST;
    }

    while(true)
    {}
}

int main()
{
    BOOT_DEBUG("boot start...\r\n");
    Set_System();
    HAL_UI_RGB_Color(RGB_COLOR_CYAN);

    HAL_PARAMS_Load_Boot_Params();
    HAL_PARAMS_Load_System_Params();

    //自动进入DFU下载模式
    if(0x7DEA != HAL_Core_Read_Backup_Register(BKP_DR_03))
    {
        //延时1.5s 等待用户进入配置模式
        delay(1500);
    }
    HAL_Core_Write_Backup_Register(BKP_DR_03, 0xFFFF);

    if(BOOTLOADER_VERSION != HAL_PARAMS_Get_Boot_boot_version())
    {
        BOOT_DEBUG("save boot version...\r\n");
        HAL_PARAMS_Set_Boot_boot_version(BOOTLOADER_VERSION);
        HAL_PARAMS_Save_Params();
    }

    if(!HAL_UI_Mode_BUTTON_GetState(BUTTON1))
    {
#define TIMING_RESERVE_MODE          3000   //保留模式
#define TIMING_DEFAULT_RESTORE_MODE  7000   //默认程序恢复判断时间
#define TIMING_SERIAL_COM_MODE       10000  //保留1模式
#define TIMING_FACTORY_RESET_MODE    13000  //恢复出厂程序判断时间 不清空密钥
#define TIMING_NC                    20000  //无操作判断时间
#define TIMING_ALL_RESET_MODE        30000  //完全恢复出厂判断时间 清空密钥
        while (!HAL_UI_Mode_BUTTON_GetState(BUTTON1))
        {
            BUTTON_press_time = HAL_UI_Mode_Button_Pressed();
            if(BUTTON_press_time > TIMING_ALL_RESET_MODE)
            {
                FACTORY_RESET_MODE = 0;
                ALL_RESET_MODE = 1;
                HAL_UI_RGB_Color(RGB_COLOR_YELLOW);
            }
            else if(BUTTON_press_time > TIMING_NC)
            {
                FACTORY_RESET_MODE = 0;
                NC_MODE = 1;
                HAL_UI_RGB_Color(0);
            }
            else if(BUTTON_press_time > TIMING_FACTORY_RESET_MODE)
            {
                SERIAL_COM_MODE=0;
                FACTORY_RESET_MODE = 1;
                HAL_UI_RGB_Color(RGB_COLOR_CYAN);
            }
            else if(BUTTON_press_time > TIMING_SERIAL_COM_MODE)
            {
                DEFAULT_FIRMWARE_MODE = 0;
                SERIAL_COM_MODE=1;
                HAL_UI_RGB_Color(RGB_COLOR_BLUE);
            }
            else if(BUTTON_press_time > TIMING_DEFAULT_RESTORE_MODE)
            {
                RESERVE_MODE=0;
                DEFAULT_FIRMWARE_MODE = 1;
                HAL_UI_RGB_Color(RGB_COLOR_GREEN);
            }
            else if(BUTTON_press_time > TIMING_RESERVE_MODE)
            {
                RESERVE_MODE=1;
                HAL_UI_RGB_Color(RGB_COLOR_RED);
            }
        }
    }
    else
    {
        switch(HAL_PARAMS_Get_Boot_boot_flag())
        {
            case 0: //正常启动
                START_APP_MODE = 1;
                break;
            case 1: //默认程序下载
                DEFAULT_FIRMWARE_MODE = 1;
                break;
            case 2: //串口通讯(保留)
                SERIAL_COM_MODE = 1;
                break;
            case 3: //恢复出厂
                FACTORY_RESET_MODE = 1;
                break;
            case 4: //在线升级
                OTA_FIRMWARE_MODE = 1;
                break;
            case 5: //完全恢复出厂  清除密钥
                ALL_RESET_MODE = 1;
                break;
            default:
                break;
        }
    }

    if(ALL_RESET_MODE)
    {
        BOOT_DEBUG("ALL factroy reset\r\n");
        Enter_Factory_ALL_RESTORE_Mode();
        ALL_RESET_MODE = 0;
    }
    else if(FACTORY_RESET_MODE)
    {
        BOOT_DEBUG("factroy reset\r\n");
        Enter_Factory_RESTORE_Mode();
        FACTORY_RESET_MODE = 0;
    }
    else if(DEFAULT_FIRMWARE_MODE)
    {
        BOOT_DEBUG("default firmware mode\r\n");
        Enter_Default_RESTORE_Mode();
        DEFAULT_FIRMWARE_MODE = 0;
    }
    else if(SERIAL_COM_MODE)
    {
        BOOT_DEBUG("serail com mode\r\n");
        Enter_Serail_Com_Mode();
        SERIAL_COM_MODE = 0;
    }
    else if(OTA_FIRMWARE_MODE)
    {
        BOOT_DEBUG("ota firmware\r\n");
        Enter_OTA_Update_Mode();
        OTA_FIRMWARE_MODE = 0;
    }
    else if(RESERVE_MODE||NC_MODE)
    {
        System_Reset();
    }
    BOOT_DEBUG("start app\r\n");
    HAL_UI_RGB_Color(RGB_COLOR_BLACK);
    Start_App();
    return 0;
}

