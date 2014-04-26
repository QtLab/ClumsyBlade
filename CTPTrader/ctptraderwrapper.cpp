#include "ctptraderwrapper.h"
#include "ctpmdwrapper.h"

#include "../ShareLib/share_headers/utils/datamanager.h"
#include "../DataAccessLayer/dataaccesslayer.h"
#include "../ShareLib/share_headers/utils/filesystemhelper.h"
#include "PVSConstant.h"

// ���ò���
//int iTradeRequestID;          //Added on July 2nd, the request ID used in ReqOrderInsert().
char FRONT_ADDR_TRADER[101];		// ǰ�õ�ַ
extern TThostFtdcBrokerIDType	BROKER_ID;				// ���͹�˾����
extern TThostFtdcInvestorIDType INVESTOR_ID;			// Ͷ���ߴ���
extern TThostFtdcPasswordType  PASSWORD;			// �û�����
extern int iRequestID;

CThostFtdcTraderApi* pTraderApi;
extern CThostFtdcMdApi* pMdApi;

//QList<QString> idList;
int counter;

void TraderThread::run()
{
    if(!pTraderApi)
    {
        qDebug() << __FUNCTION__ << "pTraderApi is null";
        return;
    }

    pTraderApi->SubscribePrivateTopic(THOST_TERT_QUICK);
    pTraderApi->SubscribePublicTopic(THOST_TERT_QUICK);
    pTraderApi->RegisterFront(FRONT_ADDR_TRADER);
    pTraderApi->Init();
    pTraderApi->Join();
}

CTPTraderWrapper::CTPTraderWrapper(QObject *parent)
    :CTPWrapper(parent)
{
    // t = NULL;
    initRes();
}

CTPTraderWrapper::~CTPTraderWrapper()
{
    idList.clear();
    releaseRes();
}

void CTPTraderWrapper::initRes()
{
    createDirInNotExist(PVSniper::TRADER_CON);

    pTraderApi = CThostFtdcTraderApi::CreateFtdcTraderApi(PVSniper::TRADER_CON.toAscii().constData());

    if(!pTraderApi){
        qDebug() << "initTraderSlot failed!";
        return;
    }

    pTraderApi->RegisterSpi(this);				// ע���¼���
}

void CTPTraderWrapper::releaseRes()
{
    if(pTraderApi)
    {
        pTraderApi->RegisterSpi(NULL);
        pTraderApi->Release();
        pTraderApi = NULL;
    }

}

void CTPTraderWrapper::FrontConnect()
{
    TraderThread *t=new TraderThread();
    t->start();

    emit outputSignal(tr("connecting to trade server..."), false);
}

void CTPTraderWrapper::OnFrontDisconnected(int nReason)
{
    qDebug() << __FUNCTION__ << " nReason" << nReason;
    emit onFrontDisconnectedSignal();
}

bool CTPTraderWrapper::IsErrorRspInfo(CThostFtdcRspInfoField *pRspInfo)
{
    // ���ErrorID != 0, ˵���յ��˴������Ӧ
    bool bResult = ((pRspInfo) && (pRspInfo->ErrorID != 0));
    if (bResult)
    {
        QString str = QString();
        str.append(pRspInfo->ErrorID + " ");
        str.append(pRspInfo->ErrorMsg);
        qDebug() << __FUNCTION__ << str;
        //emit outputSignal(str);
    }
    return bResult;
}

void CTPTraderWrapper::OnFrontConnected()
{
    emit outputSignal(tr("connect to trade server succeed!"), false);

    CThostFtdcReqUserLoginField req;
    memset(&req, 0, sizeof(req));

    strcpy(req.BrokerID, BROKER_ID);
    strcpy(req.UserID, INVESTOR_ID);
    strcpy(req.Password, PASSWORD);

    pTraderApi->ReqUserLogin(&req, ++iRequestID);
    emit outputSignal(tr("login to trade server..."), false);
    //emit OnTraderFrontConnectedSignal();
}

void CTPTraderWrapper::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{

    if (!bIsLast || IsErrorRspInfo(pRspInfo)){
        qDebug() << __FUNCTION__ << bIsLast << pRspInfo->ErrorMsg;

        emit outputSignal( pRspInfo->ErrorMsg);
        emit onLoginSignal(false);
        return;
    }

    emit outputSignal(tr("login to trade server succeed!"), false);
    emit onLoginSignal(true);

    CThostFtdcQryInstrumentField req2;
    memset(&req2, 0, sizeof(req2));
    DataManager::GetInstance()->getDAL()->resetData();
    idList.clear();
    counter = 0;

    pTraderApi->ReqQryInstrument(&req2, ++iRequestID);

}

///�����ѯ��Լ��Ӧ
void CTPTraderWrapper::OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    emit subscribeInstrumentInfoSignal(QString(pInstrument->InstrumentID), QString(pInstrument->ExchangeID));
    emit addInstrumentSignal(QString(pInstrument->InstrumentID), QString(pInstrument->ExchangeID));
}

void CTPTraderWrapper::OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    qDebug() << __FUNCTION__;
    qDebug() << pRspInfo->ErrorMsg;
    qDebug()<<"����δͨ������У��,��CTP�ܾ�";
}

void CTPTraderWrapper::OnRtnOrder(CThostFtdcOrderField *pOrder)
{
    qDebug() << __FUNCTION__;
    qDebug() << pOrder->OrderStatus;
    qDebug()<<pOrder->OrderSysID;
    if('a'==pOrder->OrderStatus)
        qDebug()<<"CTP����Order����δ����������";

    /*
#define THOST_FTDC_OST_AllTraded '0'
///���ֳɽ����ڶ�����
#define THOST_FTDC_OST_PartTradedQueueing '1'
///���ֳɽ����ڶ�����
#define THOST_FTDC_OST_PartTradedNotQueueing '2'
///δ�ɽ����ڶ�����
#define THOST_FTDC_OST_NoTradeQueueing '3'
///δ�ɽ����ڶ�����
#define THOST_FTDC_OST_NoTradeNotQueueing '4'
///����
#define THOST_FTDC_OST_Canceled '5'
///δ֪
#define THOST_FTDC_OST_Unknown 'a'
///��δ����
#define THOST_FTDC_OST_NotTouched 'b'
///�Ѵ���
#define THOST_FTDC_OST_Touched 'c'

typedef char TThostFtdcOrderStatusType;
      */


        //���Thost�����˱���ָ��û������յ�OnRspOrderInser�������յ�OnRtnOrder����������ί��״̬��

}

void CTPTraderWrapper::OnErrRtnOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo)
{
    qDebug()<<" ����������¼�����ر�";
}

void CTPTraderWrapper::OnRtnTrade(CThostFtdcTradeField *pTrade)
{
  qDebug() << __FUNCTION__;
  qDebug()<<"�ɽ�֪ͨ";

  //�������յ�������ͨ��У�顣�û����յ�OnRtnOrder��OnRtnTrade��
}

void CTPTraderWrapper::OnErrRtnOrder(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo)
{
     qDebug() << __FUNCTION__;
     qDebug() << pRspInfo->ErrorMsg;
//�����������Ϊ���������û��ͻ��յ�OnErrRtnOrder��

}
//*********************************************
//*********************************************
//*********************************************

 void CTPTraderWrapper::OnRspQrySettlementInfo(CThostFtdcSettlementInfoField *pSettlementInfo, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    qDebug()<<__FUNCTION__;
}



void CTPTraderWrapper::OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pInvestorPosition, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    qDebug() << __FUNCTION__;
}

void CTPTraderWrapper::OnRspQryTradingAccount(CThostFtdcTradingAccountField *pTradingAccount, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    qDebug() << __FUNCTION__;
    if(bIsLast&&!IsErrorRspInfo(pRspInfo))
    {
        CThostFtdcQryInvestorPositionField req;
        memset(&req, 0, sizeof(req));
        strcpy(req.BrokerID, BROKER_ID);
        strcpy(req.InvestorID, INVESTOR_ID);

        int iResult = pTraderApi->ReqQryInvestorPosition(&req, ++iRequestID);
        qDebug() << "--->>> �����ѯͶ���ֲ߳�: " << ((iResult == 0) ? "�ɹ�" : "ʧ��") << endl;

    }
}

///�����ѯ��Լ��֤������Ӧ
void CTPTraderWrapper::OnRspQryInstrumentMarginRate(CThostFtdcInstrumentMarginRateField *pInstrumentMarginRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
}

///�����ѯ��Լ����������Ӧ
void CTPTraderWrapper::OnRspQryInstrumentCommissionRate(CThostFtdcInstrumentCommissionRateField *pInstrumentCommissionRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
}

///�����ѯ��������Ӧ
void CTPTraderWrapper::OnRspQryExchange(CThostFtdcExchangeField *pExchange, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
   // qDebug() << "pExchange ExchangeID=" << pExchange->ExchangeID;
   // qDebug() << "pExchange ExchangeName=" << pExchange->ExchangeName;
   // qDebug() << "pExchange ExchangeProperty=" << pExchange->ExchangeProperty;

    QString str = QString();
    str.append(pExchange->ExchangeID);
    str.append(pExchange->ExchangeName);
    str.append(pExchange->ExchangeProperty);

    qDebug() << __FUNCTION__ << str;

    emit outputSignal(str);
}

