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

#include "intorobot_config.h"

#include <stdlib.h>
#include <stdio.h>

#include "hw_config.h"
#include "core_hal.h"
#include "params_hal.h"
#include "ota_flash_hal.h"
#include "system_datapoint.h"
#include "system_mode.h"
#include "system_task.h"
#include "system_rgbled.h"
#include "system_lorawan.h"
#include "system_product.h"
#include "system_utilities.h"
#include "string_convert.h"
#include "wiring_system.h"
#include "wiring_ext.h"

/*debug switch*/
#define SYSTEM_LORAWAN_DEBUG

#ifdef SYSTEM_LORAWAN_DEBUG
#define SLORAWAN_DEBUG(...)  do {DEBUG(__VA_ARGS__);}while(0)
#define SLORAWAN_DEBUG_D(...)  do {DEBUG_D(__VA_ARGS__);}while(0)
#else
#define SLORAWAN_DEBUG(...)
#define SLORAWAN_DEBUG_D(...)
#endif

using namespace intorobot;

#ifndef configNO_LORAWAN

#define LORAWAN_PUBLIC_NETWORK        true
#define LORAWAN_NETWORK_ID            ( uint32_t )0

volatile bool INTOROBOT_LORAWAN_SEND_INFO = false;
volatile bool INTOROBOT_LORAWAN_JOINED = false; //lorawan激活通过
volatile bool INTOROBOT_LORAWAN_CONNECTED = false; //lorawan发送版本信息完毕 已连接平台
volatile bool INTOROBOT_LORAWAN_JOIN_ENABLE = false; //入网使能 true使能
volatile bool INTOROBOT_LORAWAN_JOINING = false; //入网中

static LoRaMacPrimitives_t LoRaMacPrimitives;
static LoRaMacCallback_t LoRaMacCallbacks;
static  RadioEvents_t loraRadioEvents;

//======loramac不运行========
static void OnLoRaRadioTxDone(void)
{
    LoRa._radioRunStatus = ep_lora_radio_tx_done;
    // system_notify_event(event_lora_radio_status,ep_lora_radio_tx_done);
}

static void OnLoRaRadioTxTimeout(void)
{
    LoRa._radioRunStatus = ep_lora_radio_tx_timeout;
    // system_notify_event(event_lora_radio_status,ep_lora_radio_tx_timeout);
}

static void OnLoRaRadioRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr)
{
    LoRa._rssi = rssi;
    LoRa._snr = snr;
    LoRa._length = size;
    LoRa._radioAvailable = true;
    memcpy( LoRa._buffer, payload, LoRa._length);
    LoRa._radioRunStatus = ep_lora_radio_rx_done;
    system_notify_event(event_lora_radio_status,ep_lora_radio_rx_done,payload,size);
}

static void OnLoRaRadioRxTimeout(void)
{
    LoRa._radioRunStatus = ep_lora_radio_rx_timeout;
    // system_notify_event(event_lora_radio_status,ep_lora_radio_rx_timeout);
}

static void OnLoRaRadioRxError(void)
{
    LoRa._radioRunStatus = ep_lora_radio_rx_error;
    // system_notify_event(event_lora_radio_status,ep_lora_radio_rx_error);
}

static void OnLoRaRadioCadDone(bool channelActivityDetected)
{
    if(channelActivityDetected){
        LoRa._cadDetected = true;
    }else{
        LoRa._cadDetected = false;
    }
    LoRa._radioRunStatus = ep_lora_radio_cad_done;
    // system_notify_event(event_lora_radio_status,ep_lora_radio_cad_done,&cadDetected,1);
}

//======loramac不运行 end ========

//loramac运行回调函数
static void McpsConfirm( McpsConfirm_t *mcpsConfirm )
{
    if( mcpsConfirm->Status == LORAMAC_EVENT_INFO_STATUS_OK )
    {
        LoRaWan._macSendStatus = 1;
        switch( mcpsConfirm->McpsRequest )
        {
            case MCPS_UNCONFIRMED:
            {
                // Check Datarate
                // Check TxPower
                // DEBUG("mcpsConfirm->MCPS_UNCONFIRMED");
                LoRaWan._uplinkDatarate = mcpsConfirm->Datarate;
                LoRaWan._txPower = mcpsConfirm->TxPower;
                LoRaWan._macRunStatus = ep_lorawan_mcpsconfirm_unconfirmed;
                // system_notify_event(event_lorawan_status,ep_lorawan_mcpsconfirm_unconfirmed);
                break;
            }
            case MCPS_CONFIRMED:
            {
                // Check Datarate
                // Check TxPower
                // Check AckReceived
                // Check NbTrials
                // DEBUG("mcpsConfirm->MCPS_CONFIRMED");
                LoRaWan._uplinkDatarate = mcpsConfirm->Datarate;
                LoRaWan._txPower = mcpsConfirm->TxPower;
                LoRaWan._ackReceived = mcpsConfirm->AckReceived;
                if(!LoRaWan._ackReceived){
                    LoRaWan._macSendStatus = 2;
                }
                LoRaWan._macRunStatus = ep_lorawan_mcpsconfirm_confirmed;
                // system_notify_event(event_lorawan_status,ep_lorawan_mcpsconfirm_confirmed_ackreceived);
                // system_notify_event(event_lorawan_status,ep_lorawan_mcpsconfirm_confirmed);
                break;
            }
            case MCPS_PROPRIETARY:
            {
                // system_notify_event(event_lorawan_status,ep_lorawan_mcpsconfirm_proprietary);
                break;
            }
            default:
                break;
        }
    }
    else
    {
        if(mcpsConfirm->McpsRequest == MCPS_CONFIRMED){
            LoRaWan._macRunStatus = ep_lorawan_mcpsconfirm_confirmed;
            LoRaWan._ackReceived = false;
        }
        LoRaWan._macSendStatus = 2;
    }
}

static void McpsIndication( McpsIndication_t *mcpsIndication )
{
    if( mcpsIndication->Status != LORAMAC_EVENT_INFO_STATUS_OK )
    {
        return;
    }

    // Check Multicast
    // Check Port
    // Check Datarate
    // Check FramePending
    // Check Buffer
    // Check BufferSize
    // Check Rssi
    // Check Snr
    // Check RxSlot
    LoRaWan._downlinkDatarate = mcpsIndication->RxDatarate;
    LoRaWan._macRssi = mcpsIndication->Rssi;
    LoRaWan._macSnr = mcpsIndication->Snr;
    LoRaWan._framePending = mcpsIndication->FramePending;

    if( mcpsIndication->RxData == true )
    {
        LoRaWan.macBuffer.available = true;
        LoRaWan.macBuffer.bufferSize = mcpsIndication->BufferSize;
        memcpy(LoRaWan.macBuffer.buffer,mcpsIndication->Buffer,mcpsIndication->BufferSize);

        LoRaWanOnEvent(LORAWAN_EVENT_RX_COMPLETE);
        system_notify_event(event_lorawan_status,ep_lorawan_mcpsindication_receive_data);
    }
    else
    {
        LoRaWan.macBuffer.available = false;
    }

    switch( mcpsIndication->McpsIndication )
    {
        case MCPS_UNCONFIRMED:
        {
            // DEBUG("mcpsIndication->MCPS_UNCONFIRMED");
            // system_notify_event(event_lorawan_status,ep_lorawan_mcpsindication_unconfirmed);
            break;
        }
        case MCPS_CONFIRMED:
        {
            // DEBUG("mcpsIndication->MCPS_CONFIRMED");
            LoRaWanOnEvent(LORAWAN_EVENT_MCPSINDICATION_CONFIRMED);
            // system_notify_event(event_lorawan_status,ep_lorawan_mcpsindication_confirmed);
            break;
        }
        case MCPS_PROPRIETARY:
        {
            // system_notify_event(event_lorawan_status,ep_lorawan_mcpsindication_proprietary);
            break;
        }
        case MCPS_MULTICAST:
        {
            // system_notify_event(event_lorawan_status,ep_lorawan_mcpsindication_multicast);
            break;
        }
        default:
            break;
    }

}

static void MlmeConfirm( MlmeConfirm_t *mlmeConfirm )
{
    switch( mlmeConfirm->MlmeRequest )
    {
        case MLME_JOIN:
        {
            if( mlmeConfirm->Status == LORAMAC_EVENT_INFO_STATUS_OK )
            {
                // Status is OK, node has joined the network
                LoRaWan._macRunStatus = ep_lorawan_mlmeconfirm_join_success;
                LoRaWanOnEvent(LORAWAN_EVENT_JOINED);
                if(!System.featureEnabled(SYSTEM_FEATURE_LORAMAC_AUTO_ACTIVE_ENABLED)){
                system_notify_event(event_lorawan_status,ep_lorawan_mlmeconfirm_join_success);
                }
            }
            else
            {
                // Join was not successful. Try to join again
                LoRaWan._macRunStatus = ep_lorawan_mlmeconfirm_join_fail;
                LoRaWanOnEvent(LORAWAN_EVENT_JOIN_FAIL);
                system_notify_event(event_lorawan_status,ep_lorawan_mlmeconfirm_join_fail);
            }
            break;
        }
        case MLME_LINK_CHECK:
        {
            if( mlmeConfirm->Status == LORAMAC_EVENT_INFO_STATUS_OK )
            {
                // Check DemodMargin
                // Check NbGateways
                LoRaWan._demodMargin = mlmeConfirm->DemodMargin;
                LoRaWan._nbGateways = mlmeConfirm->NbGateways;
                // system_notify_event(event_lorawan_status,ep_lorawan_mlmeconfirm_link_check);
            }
            break;
        }
        default:
            break;
    }
}

void os_getDevEui(uint8_t *buf)
{
    char deveui[24]={0};
    HAL_PARAMS_Get_System_device_id(deveui, sizeof(deveui));
    string2hex(deveui, buf, 8, true);
}

void os_getAppEui(uint8_t *buf)
{
    char appeui[24]={0};
    HAL_PARAMS_Get_System_appeui(appeui, sizeof(appeui));
    string2hex(appeui, buf, 8, true);
}

void os_getAppKey(uint8_t *buf)
{
    char appkey[36]={0};
    HAL_PARAMS_Get_System_appkey(appkey, sizeof(appkey));
    string2hex(appkey, buf, 16, false);
}

void LoRaWanJoinEnable(bool enable)
{
    INTOROBOT_LORAWAN_JOIN_ENABLE = enable;
}

bool LoRaWanJoinIsEnabled(void)
{
    return INTOROBOT_LORAWAN_JOIN_ENABLE;
}

int8_t LoRaWanActiveStatus(void)
{
    if(INTOROBOT_LORAWAN_JOINED && INTOROBOT_LORAWAN_CONNECTED){
        return 1;
    }else{
        if(INTOROBOT_LORAWAN_JOINING){
            return 0;
        }
    }
    return -1;
}

void LoRaWanDisconnect(void)
{
    INTOROBOT_LORAWAN_JOINED = false;
    INTOROBOT_LORAWAN_JOINING = false;
    INTOROBOT_LORAWAN_JOIN_ENABLE = false;
    INTOROBOT_LORAWAN_CONNECTED = false;
    INTOROBOT_LORAWAN_SEND_INFO = false;
    #if (PLATFORM_ID == PLATFORM_ANT)
    system_rgb_blink(RGB_COLOR_GREEN, 1000);//绿灯闪烁
    #endif
}

void LoRaWanPause(void)
{
    System.disableFeature(SYSTEM_FEATURE_LORAMAC_RUN_ENABLED);
    LoRaWanJoinEnable(false);
    // Radio initialization
    // SX1276BoardInit();
    disable_irq( );
    SX1276IoIrqDeInit();
    loraRadioEvents.TxDone = OnLoRaRadioTxDone;
    loraRadioEvents.RxDone = OnLoRaRadioRxDone;
    loraRadioEvents.TxTimeout = OnLoRaRadioTxTimeout;
    loraRadioEvents.RxTimeout = OnLoRaRadioRxTimeout;
    loraRadioEvents.RxError = OnLoRaRadioRxError;
    loraRadioEvents.CadDone = OnLoRaRadioCadDone;
    Radio.Init( &loraRadioEvents );
    enable_irq( );
    Radio.SetModem( MODEM_LORA );

    DEBUG("lora radio init!!!");
    DEBUG("sync word = 0x%x",SX1276Read(0x39));
    DEBUG("sx1278 version = 0x%x", SX1276GetVersion());
    DEBUG("sx1278 freq = %d",SX1276LoRaGetRFFrequency());
}

void LoRaWanResume(void)
{
    System.enableFeature(SYSTEM_FEATURE_LORAMAC_RUN_ENABLED);
    SX1276BoardInit(); //初始化1278
    LoRaMacPrimitives.MacMcpsConfirm = McpsConfirm;
    LoRaMacPrimitives.MacMcpsIndication = McpsIndication;
    LoRaMacPrimitives.MacMlmeConfirm = MlmeConfirm;
    LoRaMacCallbacks.GetBatteryLevel = BoardGetBatteryLevel;
    LoRaMacInitialization( &LoRaMacPrimitives, &LoRaMacCallbacks );

    DEBUG("loramac init!!!");
    DEBUG("sync word = 0x%x",SX1276Read(0x39));
    DEBUG("sx1278 version = 0x%x", SX1276GetVersion());
    DEBUG("sx1278 freq = %d",SX1276LoRaGetRFFrequency());

    MibRequestConfirm_t mibReq;

    mibReq.Type = MIB_ADR;
    mibReq.Param.AdrEnable = LoRaWan.getAdrOn();
    LoRaMacMibSetRequestConfirm( &mibReq );

    mibReq.Type = MIB_PUBLIC_NETWORK;
    mibReq.Param.EnablePublicNetwork = LORAWAN_PUBLIC_NETWORK;
    LoRaMacMibSetRequestConfirm( &mibReq );
}

void LoRaWanJoinOTAA(void)
{
    MlmeReq_t mlmeReq;
    uint8_t _devEui[8];
    uint8_t _appEui[8];

    os_getDevEui(_devEui);
    os_getAppEui(_appEui);
    os_getAppKey(LoRaWan.macParams.appKey);

    memcpyr(LoRaWan.macParams.devEui,_devEui,8);
    memcpyr(LoRaWan.macParams.appEui,_appEui,8);

    mlmeReq.Type = MLME_JOIN;
    mlmeReq.Req.Join.DevEui = LoRaWan.macParams.devEui;
    mlmeReq.Req.Join.AppEui = LoRaWan.macParams.appEui;
    mlmeReq.Req.Join.AppKey = LoRaWan.macParams.appKey;
    mlmeReq.Req.Join.NbTrials = LoRaWan._joinNbTrials;

    LoRaMacMlmeRequest( &mlmeReq );
}

bool LoRaWanJoinABP(void)
{
    MibRequestConfirm_t mibReq;

    char devaddr[16] = {0}, nwkskey[36] = {0}, appskey[36] = {0};

    HAL_PARAMS_Get_System_devaddr(devaddr, sizeof(devaddr));
    HAL_PARAMS_Get_System_nwkskey(nwkskey, sizeof(nwkskey));
    HAL_PARAMS_Get_System_appskey(appskey, sizeof(appskey));
    string2hex(devaddr, (uint8_t *)&LoRaWan.macParams.devAddr, 4, true);
    string2hex(nwkskey, LoRaWan.macParams.nwkSkey, 16, false);
    string2hex(appskey, LoRaWan.macParams.appSkey, 16, false);

#if 0
    uint8_t i;
    DEBUG("devAddr = 0x%x",LoRaWan.macParams.devAddr);
    DEBUG_D("nwkSkey =");
    for( i=0;i<16;i++)
    {
        DEBUG_D("0x%x ",LoRaWan.macParams.nwkSkey[i]);
    }
    DEBUG_D("\r\n");
    DEBUG_D("appSkey =");
    for( i=0;i<16;i++)
    {
        DEBUG_D("0x%x ",LoRaWan.macParams.appSkey[i]);
    }
    DEBUG_D("\r\n");
#endif

    mibReq.Type = MIB_NET_ID;
    mibReq.Param.NetID = LORAWAN_NETWORK_ID;
    LoRaMacMibSetRequestConfirm( &mibReq );

    mibReq.Type = MIB_DEV_ADDR;
    mibReq.Param.DevAddr = LoRaWan.macParams.devAddr;
    LoRaMacMibSetRequestConfirm( &mibReq );

    mibReq.Type = MIB_NWK_SKEY;
    mibReq.Param.NwkSKey = LoRaWan.macParams.nwkSkey;
    LoRaMacMibSetRequestConfirm( &mibReq );

    mibReq.Type = MIB_APP_SKEY;
    mibReq.Param.AppSKey = LoRaWan.macParams.appSkey;
    LoRaMacMibSetRequestConfirm( &mibReq );

    mibReq.Type = MIB_NETWORK_JOINED;
    mibReq.Param.IsNetworkJoined = true;
    LoRaMacMibSetRequestConfirm( &mibReq );
    INTOROBOT_LORAWAN_CONNECTED = false;
    INTOROBOT_LORAWAN_JOINED = true;
    #if (PLATFORM_ID == PLATFORM_ANT)
    system_rgb_blink(RGB_COLOR_WHITE, 2000); //白灯闪烁
    #endif
    return true;
}

void LoRaWanGetABPParams(uint32_t &devAddr, uint8_t *nwkSkey, uint8_t *appSkey)
{
    MibRequestConfirm_t mibReq;
    LoRaMacStatus_t status;

    mibReq.Type = MIB_DEV_ADDR;
    status = LoRaMacMibGetRequestConfirm( &mibReq );
    if(status == LORAMAC_STATUS_OK)
    {
        devAddr = mibReq.Param.DevAddr;
    }

    mibReq.Type = MIB_NWK_SKEY;
    status = LoRaMacMibGetRequestConfirm( &mibReq );
    if(status == LORAMAC_STATUS_OK)
    {
        memcpy(nwkSkey,mibReq.Param.NwkSKey,16);
    }

    mibReq.Type = MIB_APP_SKEY;
    status = LoRaMacMibGetRequestConfirm( &mibReq );
    if(status == LORAMAC_STATUS_OK)
    {
        memcpy(appSkey,mibReq.Param.AppSKey,16);
    }
}

//回复平台控制类型的数据点来通知服务器节点已收到
//发送空的确认包
void LoRaWanRespondServerConfirmedFrame(void)
{
    McpsReq_t mcpsReq;
    mcpsReq.Type = MCPS_UNCONFIRMED;
    mcpsReq.Req.Unconfirmed.fBuffer = NULL;
    mcpsReq.Req.Unconfirmed.fBufferSize = 0;
    mcpsReq.Req.Unconfirmed.Datarate = LoRaWan.getDataRate();

    if( LoRaMacMcpsRequest( &mcpsReq ) == LORAMAC_STATUS_OK )
    {
        SLORAWAN_DEBUG("LoRaWan send empty frame status OK!!!");
    }
}

bool intorobot_lorawan_flag_connected(void)
{
    if (INTOROBOT_LORAWAN_JOINED && INTOROBOT_LORAWAN_CONNECTED) {
        return true;
    } else {
        return false;
    }
}

void intorobot_lorawan_send_terminal_info(void)
{
    SLORAWAN_DEBUG("---------lorawan send termianal info--------");
    int32_t index = 0, len = 0;
    uint8_t buffer[256] = {0};
    char temp[32] = {0};

    buffer[index++] = BINARY_DATA_FORMAT;
    // product_id
    buffer[index++] = 0xFF;
    buffer[index++] = 0x01;
    buffer[index++] = 0x03;
    system_get_product_id(temp, sizeof(temp));
    len = strlen(temp);
    buffer[index++] = len;
    memcpy(&buffer[index], temp, len);
    index+=len;

    // productver
    buffer[index++] = 0xFF;
    buffer[index++] = 0x02;
    buffer[index++] = 0x03;
    system_get_product_software_version(temp, sizeof(temp));
    len = strlen(temp);
    buffer[index++] = len;
    memcpy(&buffer[index], temp, len);
    index+=len;

    // board
    buffer[index++] = 0xFF;
    buffer[index++] = 0x03;
    buffer[index++] = 0x03;
    system_get_board_id(temp, sizeof(temp));
    len = strlen(temp);
    buffer[index++] = len;
    memcpy(&buffer[index], temp, len);
    index+=len;

    for(int i=0; i<index; i++)
    {
        SLORAWAN_DEBUG_D("%02x ", buffer[i]);
    }
    SLORAWAN_DEBUG_D("\r\n");

    if(LoRaWan.sendConfirmed(2, buffer, index, 120)){
        INTOROBOT_LORAWAN_CONNECTED = true;
        system_notify_event(event_lorawan_status,ep_lorawan_mlmeconfirm_join_success);
        DEBUG("termianal info send ok");
    }else{
        DEBUG("termianal info send fail");
        INTOROBOT_LORAWAN_SEND_INFO = false;
    }
}

bool intorobot_lorawan_send_data(char* buffer, uint16_t len, bool confirmed, uint16_t timeout)
{
    if(intorobot_lorawan_flag_connected()) {
        if(confirmed) {
            return LoRaWan.sendConfirmed(2, buffer, len, timeout);
        } else {
            return LoRaWan.sendUnconfirmed(2, buffer, len, timeout);
        }
    }
    return false;
}

void LoRaWanOnEvent(lorawan_event_t event)
{
    //主从模式下都由内部存储参数
    switch(event)
    {
        case LORAWAN_EVENT_JOINED:
            {
                char devaddr[16] = "", nwkskey[36] = "", appskey[36] = "";
                uint32_t devAddr;
                uint8_t nwkSkey[16],appSkey[16];

                LoRaWanGetABPParams(devAddr,nwkSkey,appSkey);
                //devaddr
                hex2string((uint8_t *)&devAddr, 4, devaddr, true);
                HAL_PARAMS_Set_System_devaddr(devaddr);
                //nwkskey
                hex2string(nwkSkey, 16, nwkskey, false);
                HAL_PARAMS_Set_System_nwkskey(nwkskey);
                //appskey
                hex2string(appSkey, 16, appskey, false);
                HAL_PARAMS_Set_System_appskey(appskey);
                HAL_PARAMS_Set_System_at_mode(AT_MODE_FLAG_OTAA_ACTIVE);
                HAL_PARAMS_Save_Params();

                SLORAWAN_DEBUG("devaddr: %s", devaddr);
                SLORAWAN_DEBUG("nwkskey: %s", nwkskey);
                SLORAWAN_DEBUG("appskey: %s", appskey);
                INTOROBOT_LORAWAN_JOINED = true;
                system_rgb_blink(RGB_COLOR_WHITE, 2000); //白灯闪烁
                SLORAWAN_DEBUG("--LoRaWanOnEvent joined--");
            }
            break;

        case LORAWAN_EVENT_JOIN_FAIL:
            INTOROBOT_LORAWAN_JOINING = false;
            SLORAWAN_DEBUG("--LoRaWanOnEvent join failed--");
            break;

        case LORAWAN_EVENT_RX_COMPLETE:
            {
                if(System.featureEnabled(SYSTEM_FEATURE_DATAPOINT_ENABLED)) //数据点使能
                {
                    int len, rssi;
                    uint8_t buffer[256];
                    len = LoRaWan.receive(buffer, sizeof(buffer), &rssi);

                    #if 0
                    SLORAWAN_DEBUG_D("lorawan receive data:");
                    for(uint16_t i=0;i<len;i++)
                    {
                        SLORAWAN_DEBUG_D("0x%x ",buffer[i]);
                    }
                    SLORAWAN_DEBUG_D("\r\n");
                    #endif

                    if(len == 0){
                        return;
                    }else{
                        intorobotParseReceiveDatapoints(buffer,len);
                    }
                    SLORAWAN_DEBUG("--LoRaWanOnEvent RX Data--");
                }
            }
            break;

        case LORAWAN_EVENT_MCPSINDICATION_CONFIRMED:
            SLORAWAN_DEBUG("LoRaWanOnEvent Respond Server ACK");
            if(LoRaWan.getMacClassType() == CLASS_C){
                LoRaWanRespondServerConfirmedFrame();
            }
            break;

        default:
            break;
    }
}

#endif
