#ifndef COMMONSTRUT_H
#define COMMONSTRUT_H

#include "../ctp_headers/ThostFtdcUserApiDataType.h"

class QString;

struct TickDataStruct
{
    QString date;
    ///��Լ����
    // TThostFtdcInstrumentIDType InstrumentID;
    ///����޸�ʱ��
    QString UpdateTime;
    ///���¼�
    TThostFtdcPriceType	LastPrice;
    ///����
    TThostFtdcVolumeType Volume;
};

#endif // COMMONSTRUT_H
