#ifndef  WIRING_LORAWAN_H_
#define  WIRING_LORAWAN_H_

#ifndef configNO_LORAWAN

#include "radio/inc/radio.h"
#include "radio/inc/sx1276.h"
#include "board/inc/sx1276-board.h"
#include "board/inc/utilities.h"
#include "board/inc/timer.h"
#include "mac/inc/LoRaMac.h"
#include "mac/inc/LoRaMacTest.h"

typedef struct {
    bool available;      //是否接收到数据 true有效
    uint16_t bufferSize; //接收的数据长度
    uint8_t buffer[256]; //缓存大小
}lorawan_data_t;

typedef struct sLoRaWanParams{
    uint32_t devAddr;
    uint8_t  nwkSkey[16];
    uint8_t  appSkey[16];
    uint8_t  devEui[8];
    uint8_t  appEui[8];
    uint8_t  appKey[16];
}lorawan_params_t;

typedef enum
{
    JOIN_ABP = 0,
    JOIN_OTAA,
}join_mode_t;

typedef enum
{
    PROTOCOL_LORAWAN = 0,
    PROTOCOL_P2P,
}protocol_mode_t;


class LoRaWanClass
{
    public:
        bool joinABP(void);                   //ABP入网
        bool joinOTAA(uint16_t timeout);      //OTAA入网激活
        bool sendConfirmed(uint8_t port, uint8_t *buffer, uint16_t len, uint16_t timeout);    //带确认发送   true:发送成功 false:发送失败
        bool sendUnconfirmed(uint8_t port, uint8_t *buffer, uint16_t len, uint16_t timeout);  //不带确认发送 true:发送成功 false:发送失败
        uint16_t receive(uint8_t *buffer, uint16_t length, int *rssi);                        //返回接收数据
        void setDeviceEUI(char *devEui);                  //设置deviceeui
        void getDeviceEUI(char *devEui, uint16_t len);    //获取deviceeui
        void setDeviceAddr(char *devAddr);                //设置device addr
        void getDeviceAddr(char *devAddr, uint16_t len);  //获取device addr
        void setAppEUI(char *appEui);                     //设置appeui
        void getAppEUI(char *appEui, uint16_t len);       //获取appeui
        void setAppKey(char *appKey);                     //设置appkey
        void setNwkSessionKey(uint8_t *nwkSkey);          //设置nwkSkey
        void setAppSessionKey(uint8_t *appSkey);          //设置appSkey
        void setMacClassType(DeviceClass_t classType);    //切换class 类型
        DeviceClass_t getMacClassType(void);              //获取class类型
        void setTxPower(uint8_t index);                   //设置发射功率
        uint8_t getTxPower(void);                         //获取发射功率
        void setDataRate(uint8_t datarate);               //设置速率
        uint8_t getDataRate(void);                        //获取速率
        void setAdrOn(bool adrOn);                        //使能ADR自适应速率 true使能 false不使能
        bool getAdrOn(void);                              //获取ADR自适应速率开关
        uint8_t getDutyCyclePrescaler(void);              //获取占空比分频参数
        void setChannelFreq(uint8_t channel, uint32_t freq);                    //设置通道频率
        uint32_t getChannelFreq(uint8_t channel);                               //获取通道频率
        void setChannelDutyCycle(uint8_t channel, uint16_t dcycle);             //设置通道占空比
        void getChannelDutyCycle(uint8_t channel);                              //设置通道占空比
        void setChannelDRRange(uint8_t channel, uint8_t minDR, uint8_t maxDR);  //设置通道速率范围
        uint8_t getChannelDRRange(uint8_t channel);                             //获取通道速率
        void setChannelStatus(uint8_t channel, bool enable);                    //设置通道使能
        bool getChannelStatus(uint8_t channel);                                 //获取通道状态
        void setConfirmedNbTrials(uint8_t trials);                              //设置带确认发送重发次数
        uint8_t getConfirmedNbTrials(void);                                     //获取带确认发送重发次数
        void setUnconfirmedNbTrials(uint8_t trials);                            //设置不带确认发送重发次数
        uint8_t getUnconfirmedNbTrials(void);                                   //获取不带确认发送重发次数
        void setJoinNbTrials(uint8_t trials);                                   //设置入网重发次数 默认最大48次
        uint8_t getJoinNbTrials(void);                                          //获取入网重发次数
        void setUpCounter(uint32_t counter);                                    //设置上行计数器
        uint32_t getUpCounter(void);                                            //获取上行计数器
        void setDownCounter(uint32_t counter);                                  //设置下行计数器
        uint32_t getDownCounter(void);                                          //获取下行计数器
        uint8_t setRX2Parameters(uint8_t datarate, uint32_t frequency);         //设置窗口2 速率和频率
        uint8_t setRX1Delay(uint16_t delay);                                    //设置窗口1 延时
        uint8_t getMargin(void);                                                //获取魔术字
        uint8_t getGatewayNumber(void);                                         //获取网关号
        uint8_t getSnr(void);                                                   //获取收到数据后的snr
        int getRssi(void);                                                      //获取收到数据后的rssi
        uint8_t sendStatus(void);

    public:
        uint8_t _buffer[256];                 //接收缓冲区
        uint16_t _length = 0;                 //接收数据长度
        uint8_t _macSnr;                      //接收完数据收snr值
        int16_t _macRssi;                     //接收完数据收rssi值
        lorawan_data_t macBuffer;
        lorawan_params_t macParams;
        DeviceClass_t _classType = CLASS_C;   //class 类型
        bool _adrOn = false;                  //ADR使能
        uint8_t _confirmedNbTrials = 3;       //确认帧重发次数
        uint8_t _unConfirmedNbTrials = 3;     //确认帧重发次数
        uint8_t _joinNbTrials = 3;            //入网包重发次数
        uint8_t _demodMargin = 0;
        uint8_t _nbGateways = 0;
        uint8_t _uplinkDatarate = 0;          //上行速率
        uint8_t _downlinkDatarate = 0;        //下行速率
        uint8_t _txPower = 0;                 //发射功率
        bool _ackReceived = false;            //是否收到ack
        uint8_t _framePending = 0;
        uint8_t _macDatarate = DR_5;          //设置发送速率
        uint8_t _macRunStatus = 0;            //mac运行状态 内部使用处理
        uint8_t _macSendStatus = 0;           //发送状态

    private:
};

class LoRaClass
{
    public:
        bool radioSend(uint8_t *buffer, uint16_t length, uint32_t timeout);                    //lora射频发送数据
        void radioStartRx(uint32_t timeout);                                                        //lora射频进入接收状态
        uint16_t radioRx(uint8_t *buffer, uint16_t length, int16_t *rssi);  //lora射频接收数据
        bool radioCad(void);                            //完成CAD流程  true:CADDeteced false:CADDone

        int16_t radioReadRssi(void);                    //获取rssi
        int8_t radioReadSnr(void);                      //获取snr

        void radioSetSleep(void);                       //sx1278休眠
        void radioSetFreq(uint32_t freq);               //设置频率
        uint32_t radioGetFreq(void);                    //获取频率
        void radioSetMaxPayloadLength(uint8_t maxLen);  //设置负载最大长度
        uint8_t radioGetMaxPayloadLength(void);         //获取负载最大长度
        void radioSetModem(RadioModems_t modem);        //设置模式 MODEM_FSK、MODEM_LORA
        RadioModems_t radioGetModem(void);              //获取模式
        void radioSetBandwidth(uint8_t bandwidth);      //设置带宽
        uint8_t radioGetBandwidth(void);                //获取带宽
        void radioSetSF(uint8_t sf);                    //设置扩频因子
        uint8_t radioGetSF(void);                       //获取扩频因子
        void radioSetCR(uint8_t coderate);              //设置纠错编码率
        uint8_t radioGetCR(void);                       //获取纠错码率
        void radioSetPreambleLen(uint16_t preambleLen); //设置前导码长度
        uint16_t radioGetPreambleLen(void);             //获取前导码长度
        void radioSetFixLenOn(bool fixLenOn);           //SF=6时包长度需固定
        bool radioGetFixLenOn(void);                    //SF=6时包长度需固定
        void radioSetCrcOn(bool crcOn);                 //设置crc校验
        bool radioGetCrcOn(void);                       //获取crc是否开启 0:关闭1:开启
        void radioSetFreqHopOn(bool FreqHopOn);         //设置跳频使能
        bool radioGetFreqHopOn(void);                   //获取跳频使能
        void radioSetHopPeriod(uint8_t hopPeriod);      //设置跳频周期
        uint8_t radioGetHopPeriod(void);                //获取跳频周期
        void radioSetIqInverted(bool iqInverted);       //设置iq翻转
        bool radioGetIqInverted(void);                  //获取Iq是否翻转
        void radioSetRxContinuous(bool rxContinuous);
        bool radioGetRxContinuous(void);

        //发送射频相关参数
        void radioSetTxPower(uint8_t txPower);          //设置发射功率
        uint8_t radioGetTxPower(void);                  //获取发射功率

        //接收射频相关参数
        void radioSetFixPayloadLen(uint8_t payloadLen); //SF=6时包长度需固定,设置包长度
        void radioSetSymbTimeout(uint32_t symbTimeout); //设置前导码超时
        uint32_t radioGetSymbTimeout(void);             //获取前导码超时值

        uint8_t radioRead(uint8_t addr);                //读取寄存器值
        void radioWrite(uint8_t addr, uint8_t data);    //写寄存器值

    public:
        uint8_t _buffer[256];                //接收缓冲区
        uint16_t _length = 0;                //接收数据长度
        int8_t _snr;                         //接收完数据收snr值
        int16_t _rssi;                       //接收完数据收rssi值
        uint8_t _radioRunStatus = 0;         //发送接收状态
        bool _cadDetected = false;           //是否检测到cad
        bool _radioAvailable = false;        //是否收到数据

    private:
        uint32_t _freq = 434000000;          //工作频率
        uint8_t _maxPayloadLen = 0x40;       //最大负载长度
        RadioModems_t _modem = MODEM_LORA;   //模式 MODEM_FSK:fsk   MODEM_LORA:lora
        uint32_t _bandwidth  = 0;            //带宽fsk:2600-250000Hz lora:0-125KHz 1-250K 2-500K
        uint32_t _datarate   = 7;            //速率fsk:600-300000　lora:扩频因子 6-12
        uint8_t _coderate    = 1;            //纠错码率 fsk:0 lora: 1(4/5) 2(4/6) 3(4/7) 4(4/8) 仅lora用
        uint32_t _bandwidthAfc = 0;          //fsk:2600-250000 lora:0 仅fsk用
        uint16_t _preambleLen  = 8;          //前导码长度
        uint16_t _symbTimeout  = 8;          //单次接收超时
        bool _fixLenOn = false;              //固定包长 true:固定　false:无效
        uint8_t _payloadLen = 0;             //当fixLen设为1时才有效,设置包长度
        bool _crcOn = true;                  //crc 校验 1:on 0:off
        bool _freqHopOn = false;             //内部跳频 fsk无效,设为0 lora:1-on 0-off
        uint8_t _hopPeriod = 0;              //跳频周期 单位symbols fsk:0 lora: number of symbols
        bool _iqInverted = false;            //IQ位翻转 1:翻转 0:不翻转
        bool _rxContinuous = true;           //连续模式
        int8_t _power = 20;                  //发射功率
        uint32_t _fdev = 0;                  //频率偏差　仅fsk有效 fsk:Hz lora:0
        uint32_t _txTimeout = 3000;          //发射超时时间 单位ms
};

extern LoRaWanClass LoRaWan;
extern LoRaClass LoRa;

#endif
#endif /* WIRING_LORAWAN_H_ */

