/**
 ******************************************************************************
 Copyright (c) 2013-2014 IntoRobot Team.  All right reserved.

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation, either
 version 3 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */
#include "hw_config.h"
#include "ui_hal.h"


#define RGB_R_GPIO_PIN       GPIO_PIN_0
#define RGB_R_GPIO_PORT      GPIOA
#define RGB_G_GPIO_PIN       GPIO_PIN_1
#define RGB_G_GPIO_PORT      GPIOA
#define RGB_B_GPIO_PIN       GPIO_PIN_12
#define RGB_B_GPIO_PORT      GPIOB

#define MODE_BOTTON_GPIO_PIN       GPIO_PIN_2
#define MODE_BOTTON_GPIO_PORT      GPIOB


volatile uint32_t BUTTON_last_state = 0;
volatile uint32_t TimingBUTTON=0;
volatile uint32_t TimingLED;
rgb_info_t rgb_info;

void Set_RGB_Color(uint32_t color) {
    uint8_t red,green,blue;

    red = color>>16 & 0xFF;
    green = color>>8 & 0xFF;
    blue = color & 0xFF;
    if(red) {
        HAL_GPIO_WritePin(RGB_R_GPIO_PORT, RGB_R_GPIO_PIN, GPIO_PIN_RESET);
    } else {
        HAL_GPIO_WritePin(RGB_R_GPIO_PORT, RGB_R_GPIO_PIN, GPIO_PIN_SET);
    }

    if(green) {
        HAL_GPIO_WritePin(RGB_G_GPIO_PORT, RGB_G_GPIO_PIN, GPIO_PIN_RESET);
    } else {
        HAL_GPIO_WritePin(RGB_G_GPIO_PORT, RGB_G_GPIO_PIN, GPIO_PIN_SET);
    }

    if(blue) {
        HAL_GPIO_WritePin(RGB_B_GPIO_PORT, RGB_B_GPIO_PIN, GPIO_PIN_RESET);
    } else {
        HAL_GPIO_WritePin(RGB_B_GPIO_PORT, RGB_B_GPIO_PIN, GPIO_PIN_SET);
    }
}

void RGB_Color_Toggle(void) {
    if(rgb_info.rgb_last_color) {
        Set_RGB_Color(0);
        rgb_info.rgb_last_color = 0;
    } else {
        Set_RGB_Color(rgb_info.rgb_color);
        rgb_info.rgb_last_color = rgb_info.rgb_color;
    }
}

void HAL_UI_Initial(void)
{
    //三色灯管脚初始化
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef  GPIO_InitStruct;
    GPIO_InitStruct.Pin = RGB_R_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
    HAL_GPIO_Init(RGB_R_GPIO_PORT, &GPIO_InitStruct);
    GPIO_InitStruct.Pin = RGB_G_GPIO_PIN;
    HAL_GPIO_Init(RGB_G_GPIO_PORT, &GPIO_InitStruct);
    GPIO_InitStruct.Pin = RGB_B_GPIO_PIN;
    HAL_GPIO_Init(RGB_B_GPIO_PORT, &GPIO_InitStruct);

    HAL_GPIO_WritePin(RGB_R_GPIO_PORT, RGB_R_GPIO_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(RGB_G_GPIO_PORT, RGB_G_GPIO_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(RGB_B_GPIO_PORT, RGB_B_GPIO_PIN, GPIO_PIN_SET);

    //侧边配置按键管脚初始化
    GPIO_InitStruct.Pin = MODE_BOTTON_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
    HAL_GPIO_Init(MODE_BOTTON_GPIO_PORT, &GPIO_InitStruct);
}

uint8_t HAL_UI_Mode_BUTTON_GetState(Button_TypeDef Button)
{
    return HAL_GPIO_ReadPin(MODE_BOTTON_GPIO_PORT, MODE_BOTTON_GPIO_PIN);
}

uint32_t HAL_UI_Mode_Button_Pressed(void)
{
    return TimingBUTTON;
}

int HAL_UI_RGB_Get_Info(rgb_info_t *pinfo)
{
    memcpy(pinfo, &rgb_info, sizeof(rgb_info_t));
    return 0;
}

int HAL_UI_RGB_Set_Info(rgb_info_t info)
{
    memcpy(&rgb_info, &info, sizeof(rgb_info_t));
    return 0;
}

void HAL_UI_RGB_Color(uint32_t color)
{
    memset(&rgb_info, 0, sizeof(rgb_info));
    rgb_info.rgb_mode = RGB_MODE_COLOR;
    rgb_info.rgb_color = color;
    Set_RGB_Color(color);
}

void HAL_UI_RGB_Blink(uint32_t color, uint16_t period)
{
    memset(&rgb_info, 0, sizeof(rgb_info));
    rgb_info.rgb_mode = RGB_MODE_BLINK;
    rgb_info.rgb_color = color;
    rgb_info.rgb_period = period>100 ? period >> 1 : 50;
}

void HAL_UI_RGB_Breath(uint32_t color, uint16_t period)
{
    HAL_UI_RGB_Blink(color, period);
}

void HAL_UI_UserLED_Control(uint8_t value)
{
}

void HAL_UI_SysTick_Handler(void)
{
    //三色灯处理
    if(RGB_MODE_BLINK == rgb_info.rgb_mode) {
        if (TimingLED != 0x00) {
            TimingLED--;
        } else {
            RGB_Color_Toggle();
            TimingLED = rgb_info.rgb_period;
        }
    }
    //侧边按键处理
    if(!HAL_UI_Mode_BUTTON_GetState(BUTTON1)) {
        if(!BUTTON_last_state) {
            TimingBUTTON = 0;
            BUTTON_last_state = 1;
        } else {
            TimingBUTTON++;
        }
    } else {
        if(BUTTON_last_state) {
            TimingBUTTON = 0;
            BUTTON_last_state = 0;
        }
    }
}

