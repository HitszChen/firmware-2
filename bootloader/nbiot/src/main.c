/**
******************************************************************************
* @file    USB_Device/DFU_Standalone/Src/main.c
* @author  MCD Application Team
* @version V1.3.2
* @date    13-November-2015
* @brief   USB device DFU demo main file
******************************************************************************
* @attention
*
* <h2><center>&copy; COPYRIGHT(c) 2015 STMicroelectronics</center></h2>
*
* Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
* You may not use this file except in compliance with the License.
* You may obtain a copy of the License at:
*
*        http://www.st.com/software_license_agreement_liberty_v2
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
******************************************************************************
*/

/* Includes ------------------------------------------------------------------*/
#include "hw_config.h"
#include "system_config.h"
#include "boot_mode.h"
#include "params_hal.h"
#include "ui_hal.h"
#include "bkpreg_hal.h"
#include "boot_debug.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define BOOTLOADER_VERSION  1

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
uint32_t BUTTON_press_time=0;

uint8_t USB_DFU_MODE          = 0;
uint8_t FACTORY_RESET_MODE    = 0;
uint8_t START_APP_MODE        = 0;


/**
 * @brief  Main program
 * @param  None
 * @retval None
 */
int main(void)
{
    HAL_System_Config();
    BOOT_DEBUG("boot start...\r\n");

    HAL_PARAMS_Load_Boot_Params();
    HAL_PARAMS_Load_System_Params();

    if(0x7DEA != HAL_Core_Read_Backup_Register(BKP_DR_03))
    {
        //延时2s 等待用户进入配置模式 和 等待用户st-link烧写程序
        for(int i = 0; i < 20; i++) {
            if(!HAL_UI_Mode_BUTTON_GetState(BUTTON1)) {
                break;
            }
            delay(100);
        }
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
#define TIMING_DFU_DOWNLOAD_MODE     1000   //dfu 下载模式
        while (!HAL_UI_Mode_BUTTON_GetState(BUTTON1))
        {
            BUTTON_press_time = HAL_UI_Mode_Button_Pressed();
            if( BUTTON_press_time > TIMING_DFU_DOWNLOAD_MODE )
            {
                USB_DFU_MODE = 1;
                HAL_UI_RGB_Color(RGB_COLOR_MAGENTA);
            }
        }
    }
    else
    {
        switch(HAL_PARAMS_Get_Boot_boot_flag())
        {
            case BOOT_FLAG_NORMAL:          //正常启动
                START_APP_MODE = 1;
                break;
            case BOOT_FLAG_FACTORY_RESET:   //恢复出厂
                FACTORY_RESET_MODE = 1;
                break;
            case BOOT_FLAG_USB_DFU:         //进入DFU下载模式
                USB_DFU_MODE = 1;
                break;
            default:
                break;
        }
    }

    //自动进入DFU下载模式
    if(0x7DEA == HAL_Core_Read_Backup_Register(BKP_DR_01))
    {
        USB_DFU_MODE = 1;
    }
    HAL_Core_Write_Backup_Register(BKP_DR_01, 0xFFFF);

    if(FACTORY_RESET_MODE)
    {
        BOOT_DEBUG("factroy reset\r\n");
        Enter_Factory_RESTORE_Mode();
        FACTORY_RESET_MODE = 0;
    }
    else if(USB_DFU_MODE)
    {
        BOOT_DEBUG("dfu\r\n");
        Enter_DFU_Mode();
    }

    BOOT_DEBUG("start app\r\n");
    HAL_UI_RGB_Color(RGB_COLOR_BLACK); //防止进入应用程序初始化三色灯 导致闪灯
    start_app();
    return 0;
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
