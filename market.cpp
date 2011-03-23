#include "stdafx.h"
#include "serv.h"
#include "market.h"
#include "trans2quik_api.h"

static DWORD   s_dwThreadId = 0;
static CMarketServer* s_pMarket = NULL;

BOOL WINAPI ConsoleHandler(DWORD CEvent)
{
    switch(CEvent)
    {
		case CTRL_C_EVENT:
		case CTRL_BREAK_EVENT:
		case CTRL_CLOSE_EVENT:
		case CTRL_LOGOFF_EVENT:
		case CTRL_SHUTDOWN_EVENT:
			::PostThreadMessage( s_dwThreadId, WM_QUIT, 0, 0 );
        break;
    }
    return TRUE;
}

extern "C" void __stdcall TRANS2QUIK_ConnectionStatusCallback (long nConnectionEvent, long nExtendedErrorCode, LPCSTR lpcstrInfoMessage)
{
	if( s_pMarket ) 
	{
			if (nConnectionEvent == TRANS2QUIK_QUIK_CONNECTED) 
				s_pMarket->OnConnected();
			if (nConnectionEvent == TRANS2QUIK_QUIK_DISCONNECTED) 
				s_pMarket->OnDisconnected();
	}
}

extern "C" void __stdcall TRANS2QUIK_TransactionsReplyCallback (long nTransactionResult, long nTransactionExtendedErrorCode, long nTransactionReplyCode, DWORD dwTransId, double dOrderNum, LPCSTR lpcstrTransactionReplyMessage)
{
	if( s_pMarket ) {
		s_pMarket->OnTransactionResult(nTransactionResult,nTransactionExtendedErrorCode, nTransactionReplyCode, dwTransId, dOrderNum, lpcstrTransactionReplyMessage);
	}
}

Market::Market() : m_pDataSource( new CMarketServer() ) {
	s_pMarket = m_pDataSource;
	m_szErrorMessage[0] = 0;
	m_nResult = m_nExtendedErrorCode = 0;
	m_pDataSource->Create( DDE_SERVER_NAME );
}

Market::~Market() {
	m_pDataSource->Shutdown();
	delete m_pDataSource;
}

void Market::addListener( MarketListener* pListener ) {
	m_pDataSource->m_listeners.push_back( pListener );
}

void Market::removeListener( MarketListener* pListener ) {
	m_pDataSource->m_listeners.remove( pListener );
}


long Market::connect(const char* quickDir) {
	m_nResult = TRANS2QUIK_SET_CONNECTION_STATUS_CALLBACK (TRANS2QUIK_ConnectionStatusCallback, &m_nExtendedErrorCode, m_szErrorMessage, sizeof (m_szErrorMessage));
	if (m_nResult != TRANS2QUIK_SUCCESS) return m_nResult;
	
	m_nResult = TRANS2QUIK_SET_TRANSACTIONS_REPLY_CALLBACK (TRANS2QUIK_TransactionsReplyCallback, &m_nExtendedErrorCode, m_szErrorMessage, sizeof (m_szErrorMessage));
	if (m_nResult != TRANS2QUIK_SUCCESS) return m_nResult;

	m_nResult = TRANS2QUIK_CONNECT ((char*)quickDir, &m_nExtendedErrorCode, m_szErrorMessage, sizeof (m_szErrorMessage));
	return m_nResult;
}

long Market::disconnect() {
	m_nResult = TRANS2QUIK_DISCONNECT(&m_nExtendedErrorCode, m_szErrorMessage, sizeof (m_szErrorMessage));
	return m_nResult;
}

long Market::sendAsync(const char* trans) {
	m_nResult = TRANS2QUIK_SEND_ASYNC_TRANSACTION ((char*)trans, &m_nExtendedErrorCode, m_szErrorMessage, sizeof (m_szErrorMessage));
	return m_nResult;
}

void Market::run() 
{
	s_dwThreadId = m_dwThreadId = ::GetCurrentThreadId();
	if(SetConsoleCtrlHandler( (PHANDLER_ROUTINE)ConsoleHandler,TRUE)==FALSE)
    {
        ASSERT("Unable to install handler" == 0);
    }

	MSG msg;
	int err;
	
	while( (err = GetMessage(&msg,NULL,0,0)) != 0 )
	{
		if( err == -1 ) 
			break;
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

void Market::stop() {
	::PostThreadMessage( m_dwThreadId, WM_QUIT, 0, 0 );
}

Table::Table() {
}

Table::~Table() {
}

void Table::init(int r, int c) {
	stable.init( r, c, "" );
	dtable.init( r, c, 0.0 );
}

void Table::reset() {
	stable.reset();
	dtable.reset();
}

int Table::cols() { 
	return stable.cols; 
}

int Table::rows() { 
	return stable.rows; 
}

const char* Table::getString(int r, int c) {
	return stable[r][c].c_str();
}

double Table::getDouble(int r, int c) {
	return dtable[r][c];
}

void Table::setString(int r, int c, const char* val) {
	stable[r][c] = string(val);
}

void Table::setDouble(int r, int c, double val) {
	dtable[r][c] = val;
}