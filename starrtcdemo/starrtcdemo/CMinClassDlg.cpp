// CMinClassDlg.cpp: 实现文件
//

#include "stdafx.h"
#include "starrtcdemo.h"
#include "CMinClassDlg.h"
#include "afxdialogex.h"
#include "CInterfaceUrls.h"
#include "CWhitePanelDlg.h"
#include "json.h"
#define WM_RECV_MIN_CLASS_MSG WM_USER + 3
#define WM_RECV_WHITE_PANEL_MSG WM_USER + 4

//UTF8转ANSI
string UTF8toANSI(string strUTF8)
{
	string retStr = "";
	//获取转换为多字节后需要的缓冲区大小，创建多字节缓冲区
	UINT nLen = MultiByteToWideChar(CP_UTF8, NULL, strUTF8.c_str(), -1, NULL, NULL);
	WCHAR *wszBuffer = new WCHAR[nLen + 1];
	nLen = MultiByteToWideChar(CP_UTF8, NULL, strUTF8.c_str(), -1, wszBuffer, nLen);
	wszBuffer[nLen] = 0;

	nLen = WideCharToMultiByte(936, NULL, wszBuffer, -1, NULL, NULL, NULL, NULL);
	CHAR *szBuffer = new CHAR[nLen + 1];
	nLen = WideCharToMultiByte(936, NULL, wszBuffer, -1, szBuffer, nLen, NULL, NULL);
	szBuffer[nLen] = 0;

	retStr = szBuffer;
	//清理内存
	delete[]szBuffer;
	delete[]wszBuffer;
	return retStr;
}
//ANSI转UTF8
string ANSItoUTF8(string strAnsi)
{
	string retStr = "";
	//获取转换为宽字节后需要的缓冲区大小，创建宽字节缓冲区，936为简体中文GB2312代码页
	UINT nLen = MultiByteToWideChar(936, NULL, strAnsi.c_str(), -1, NULL, NULL);
	WCHAR *wszBuffer = new WCHAR[nLen + 1];
	nLen = MultiByteToWideChar(936, NULL, strAnsi.c_str(), -1, wszBuffer, nLen);
	wszBuffer[nLen] = 0;
	//获取转为UTF8多字节后需要的缓冲区大小，创建多字节缓冲区
	nLen = WideCharToMultiByte(CP_UTF8, NULL, wszBuffer, -1, NULL, NULL, NULL, NULL);
	CHAR *szBuffer = new CHAR[nLen + 1];
	nLen = WideCharToMultiByte(CP_UTF8, NULL, wszBuffer, -1, szBuffer, nLen, NULL, NULL);
	szBuffer[nLen] = 0;

	retStr = szBuffer;
	//内存清理
	delete[]wszBuffer;
	delete[]szBuffer;
	return retStr;
}


DWORD WINAPI SetShowMinClassPicThread(LPVOID p)
{
	CMinClassDlg* pMinClassDlg = (CMinClassDlg*)p;

	while (WaitForSingleObject(pMinClassDlg->m_hSetShowPicEvent, INFINITE) == WAIT_OBJECT_0)
	{
		if (pMinClassDlg->m_bExit)
		{
			break;
		}
		if (pMinClassDlg->m_pDataShowView != NULL)
		{
			pMinClassDlg->m_pDataShowView->setShowPictures();
		}
	}
	return 0;
}
// CMinClassDlg 对话框

IMPLEMENT_DYNAMIC(CMinClassDlg, CDialogEx)

CMinClassDlg::CMinClassDlg(CUserManager* pUserManager, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_MIN_CLASS, pParent)
{
	m_pUserManager = pUserManager;
	m_pConfig = NULL;
	m_pLiveManager = new XHLiveManager(this);
	m_pSoundManager = new CSoundManager(this);
	m_pDataShowView = NULL;

	m_hSetShowPicThread = NULL;
	m_hSetShowPicEvent = NULL;
	m_pCurrentLive = NULL;
	m_bWatch = false;

	//	m_pWhitePanel = NULL;
}

CMinClassDlg::~CMinClassDlg()
{
	m_bStop = true;
	m_bExit = true;
	m_pConfig = NULL;
	if (m_pSoundManager != NULL)
	{
		delete m_pSoundManager;
		m_pSoundManager = NULL;
	}

	if (m_pLiveManager != NULL)
	{
		delete m_pLiveManager;
		m_pLiveManager = NULL;
	}

	if (m_pDataShowView != NULL)
	{
		delete m_pDataShowView;
		m_pDataShowView = NULL;
	}

	if (m_hSetShowPicEvent != NULL)
	{
		SetEvent(m_hSetShowPicEvent);
		m_hSetShowPicEvent = NULL;
	}
	if (m_pCurrentLive != NULL)
	{
		delete m_pCurrentLive;
		m_pCurrentLive = NULL;
	}

	//if (m_pWhitePanel != NULL)
	//{
	//	delete m_pWhitePanel;
	//	m_pWhitePanel = NULL;
	//}
}

void CMinClassDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_STATIC_MIN_CLASS_LIVE_AREA, m_LiveArea);
	DDX_Control(pDX, IDC_LIST_MIN_CLASS_MSG_LIST, m_HistoryMsg);
	DDX_Control(pDX, IDC_EDIT_MIN_CLASS_SEND_MSG, m_SendMsg);
	DDX_Control(pDX, IDC_STATIC_MIN_CLASS_NAME, m_MinClassName);
	DDX_Control(pDX, IDC_STATIC_MIN_CLASS_WHITE_PANEL_AREA, m_WhitePanelArea);
}


BEGIN_MESSAGE_MAP(CMinClassDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON_MIN_CLASS_SEND_MSG, &CMinClassDlg::OnBnClickedButtonMinClassSendMsg)
	ON_BN_CLICKED(IDC_BUTTON_MIN_CLASS_CLEAR, &CMinClassDlg::OnBnClickedButtonMinClassClear)
	ON_BN_CLICKED(IDC_BUTTON_MIN_CLASS_REVOKE, &CMinClassDlg::OnBnClickedButtonMinClassRevoke)
	ON_BN_CLICKED(IDC_BUTTON_MIN_CLASS_SET_COLOR, &CMinClassDlg::OnBnClickedButtonMinClassSetColor)
	ON_BN_CLICKED(IDC_BUTTON_MIN_CLASS_STYLE, &CMinClassDlg::OnBnClickedButtonMinClassStyle)
	ON_MESSAGE(WM_RECV_MIN_CLASS_MSG, OnRecvMinClassMsg)
	ON_MESSAGE(WM_RECV_WHITE_PANEL_MSG, OnRecvWhitePanelMsg)
	ON_WM_MOVE()
	ON_BN_CLICKED(IDC_BUTTON_MIN_CLASS_APPLY_UPLOAD, &CMinClassDlg::OnBnClickedButtonMinClassApplyUpload)
	ON_BN_CLICKED(IDC_BUTTON_MIN_CLASS_EXIT_UPLOAD, &CMinClassDlg::OnBnClickedButtonMinClassExitUpload)
END_MESSAGE_MAP()


// CMinClassDlg 消息处理程序

BOOL CMinClassDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_WhitePanelDlg.Create(IDD_DIALOG_WHITE_PANEL, this);
	m_WhitePanelDlg.setWhitePanelCallback(this, m_pUserManager->m_ServiceParam.m_strUserId);
	//	m_WhitePanelDlg.laserPenOn();
		//CWhitePanelDlg dlg;
		//dlg.DoModal();
	CRect dlgRect;
	::GetWindowRect(this->m_hWnd, dlgRect);

	//static CStarWhitePanel min;
	CRect whitePanelArea;
	m_WhitePanelArea.GetWindowRect(whitePanelArea);

	CRect showWhitePanelRect;
	showWhitePanelRect.left = whitePanelArea.left - dlgRect.left;
	showWhitePanelRect.top = whitePanelArea.top - dlgRect.top - 25;
	showWhitePanelRect.right = showWhitePanelRect.left + whitePanelArea.Width();
	showWhitePanelRect.bottom = showWhitePanelRect.top + whitePanelArea.Height();

	CRect rect;
	::GetWindowRect(m_LiveArea, rect);

	int left = rect.left - dlgRect.left - 7;
	int top = rect.top - dlgRect.top - 25;

	CRect showRect(left, top, left + rect.Width() - 5, top + rect.Height() - 15);
	m_hSetShowPicEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_pDataShowView = new CMinClassDataShowView();
	m_pDataShowView->setDrawRect(showRect);

	CPicControl *pPicControl = new CPicControl();
	pPicControl->Create(_T(""), WS_CHILD | WS_VISIBLE | SS_BITMAP, showRect, this, WM_USER + 100);
	//mShowPicControlVector[i] = pPicControl;
	pPicControl->ShowWindow(SW_SHOW);
	DWORD dwStyle = ::GetWindowLong(pPicControl->GetSafeHwnd(), GWL_STYLE);
	::SetWindowLong(pPicControl->GetSafeHwnd(), GWL_STYLE, dwStyle | SS_NOTIFY);

	m_pDataShowView->m_pPictureControlArr.push_back(pPicControl);
	pPicControl->setInfo(NULL);

	CRect rectClient = showRect;
	CRect rectChild(rectClient.right - (int)(rectClient.Width()*0.25), rectClient.top, rectClient.right, rectClient.bottom);

	for (int n = 0; n < 6; n++)
	{
		CPicControl *pPictureControl = new CPicControl();
		pPictureControl->setInfo(NULL);
		rectChild.top = rectClient.top + (long)(n * rectClient.Height()*0.25);
		rectChild.bottom = rectClient.top + (long)((n + 1) * rectClient.Height()*0.25);
		pPictureControl->Create(_T(""), WS_CHILD | WS_VISIBLE | SS_BITMAP, rectChild, this, WM_USER + 100 + n + 1);
		m_pDataShowView->m_pPictureControlArr.push_back(pPictureControl);
		DWORD dwStyle1 = ::GetWindowLong(pPictureControl->GetSafeHwnd(), GWL_STYLE);
		::SetWindowLong(pPictureControl->GetSafeHwnd(), GWL_STYLE, dwStyle1 | SS_NOTIFY);
	}

	m_hSetShowPicThread = CreateThread(NULL, 0, SetShowMinClassPicThread, (void*)this, 0, 0); // 创建线程
	//m_ApplyUploadButton.EnableWindow(FALSE);
	//m_ExitUpload.EnableWindow(FALSE);
	bool bRet = true;
	if (m_bWatch)
	{
		GetDlgItem(IDC_BUTTON_MIN_CLASS_APPLY_UPLOAD)->EnableWindow(m_bWatch);
		GetDlgItem(IDC_BUTTON_MIN_CLASS_EXIT_UPLOAD)->EnableWindow(!m_bWatch);
		bRet = watchMinClass();
	}
	else
	{
		GetDlgItem(IDC_BUTTON_MIN_CLASS_APPLY_UPLOAD)->EnableWindow(false);
		GetDlgItem(IDC_BUTTON_MIN_CLASS_EXIT_UPLOAD)->EnableWindow(false);
		bRet = startMinClass();
	}
	if (!bRet)
	{
		AfxMessageBox("加入小班课失败！");
		PostMessage(WM_CLOSE, 0, 0);
	}

	

	GetDlgItem(IDC_BUTTON_MIN_CLASS_STYLE)->EnableWindow(!m_bWatch);
	GetDlgItem(IDC_BUTTON_MIN_CLASS_SET_COLOR)->EnableWindow(!m_bWatch);
	GetDlgItem(IDC_BUTTON_MIN_CLASS_REVOKE)->EnableWindow(!m_bWatch);
	GetDlgItem(IDC_BUTTON_MIN_CLASS_CLEAR)->EnableWindow(!m_bWatch);
	
	m_MinClassName.SetWindowText(m_pCurrentLive->m_strName);
	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}

void CMinClassDlg::OnBnClickedButtonMinClassSendMsg()
{
	CString strMsg = "";
	m_SendMsg.GetWindowText(strMsg);

	if (strMsg == "")
	{
		return;
	}
	if (m_pLiveManager != NULL)
	{
		CIMMessage* pIMMessage = m_pLiveManager->sendMessage(strMsg.GetBuffer(0));
		CString strMsg = "";
		strMsg.Format("%s:%s", pIMMessage->m_strFromId.c_str(), pIMMessage->m_strContentData.c_str());
		m_HistoryMsg.InsertString(m_HistoryMsg.GetCount(), strMsg);
		delete pIMMessage;
		pIMMessage = NULL;
		m_SendMsg.SetSel(0, -1); // 选中所有字符
		m_SendMsg.ReplaceSel(_T(""));
	}
}

void CMinClassDlg::OnBnClickedButtonMinClassClear()
{
	m_WhitePanelDlg.clear();
	char buf[128] = { 0 };
	string strInfo = "{\"account\":\"";
	strInfo += m_pUserManager->m_ServiceParam.m_strUserId;
	strInfo += "\",\"data\":\"";
	sprintf_s(buf, "%d", WHITE_PANEL_CLEAR);
	strInfo += buf;


	strInfo += ":";
	memset(buf, 0, sizeof(buf));
	sprintf_s(buf, "%d", 0);
	strInfo += buf;

	strInfo += ",";
	memset(buf, 0, sizeof(buf));
	sprintf_s(buf, "%d", 0);
	strInfo += buf;

	strInfo += "\"}";

	strInfo = ANSItoUTF8(strInfo);
	if (m_pLiveManager != NULL)
	{
		m_pLiveManager->insertRealtimeData((uint8_t*)strInfo.c_str(), strInfo.length());
	}
}


void CMinClassDlg::OnBnClickedButtonMinClassRevoke()
{
	m_WhitePanelDlg.revoke(m_pUserManager->m_ServiceParam.m_strUserId);
	char buf[128] = { 0 };
	string strInfo = "{\"account\":\"";
	strInfo += m_pUserManager->m_ServiceParam.m_strUserId;
	strInfo += "\",\"data\":\"";
	sprintf_s(buf, "%d", WHITE_PANEL_REVOKE);
	strInfo += buf;

	strInfo += ":";
	memset(buf, 0, sizeof(buf));
	sprintf_s(buf, "%d", 0);
	strInfo += buf;

	strInfo += ",";
	memset(buf, 0, sizeof(buf));
	sprintf_s(buf, "%d", 0);
	strInfo += buf;

	strInfo += "\"}";

	strInfo = ANSItoUTF8(strInfo);
	if (m_pLiveManager != NULL)
	{
		m_pLiveManager->insertRealtimeData((uint8_t*)strInfo.c_str(), strInfo.length());
	}
}


void CMinClassDlg::OnBnClickedButtonMinClassSetColor()
{
	CColorDialog dlg;
	dlg.m_cc.Flags |= CC_FULLOPEN | CC_RGBINIT;
	dlg.m_cc.rgbResult = RGB(255, 0, 0);
	//打开颜色对话框,获取选定的颜色 
	if (dlg.DoModal() == IDOK)
	{
		DWORD nColor = dlg.GetColor();;
		//int nColor = ccolor[0];
		int r = GetRValue(nColor);
		int g = GetGValue(nColor);
		int b = GetBValue(nColor);
		nColor = ((COLORREF)(((BYTE)(b) | ((WORD)((BYTE)(g)) << 8)) | (((DWORD)(BYTE)(r)) << 16)));
		m_WhitePanelDlg.setLineColor(nColor);
	}
}


void CMinClassDlg::OnBnClickedButtonMinClassStyle()
{
	CString str;
	GetDlgItem(IDC_BUTTON_MIN_CLASS_STYLE)->GetWindowText(str);
	if (str == "画笔")
	{
		m_WhitePanelDlg.laserPenOn();
		str = "激光笔";
		GetDlgItem(IDC_BUTTON_MIN_CLASS_STYLE)->SetWindowText(str);
	}
	else
	{
		m_WhitePanelDlg.laserPenOff();
		str = "画笔";
		GetDlgItem(IDC_BUTTON_MIN_CLASS_STYLE)->SetWindowText(str);

		char buf[128] = { 0 };
		string strInfo = "{\"account\":\"";
		strInfo += m_pUserManager->m_ServiceParam.m_strUserId;
		strInfo += "\",\"data\":\"";
		sprintf_s(buf, "%d", WHITE_PANEL_LASER_PEN_END);
		strInfo += buf;

		strInfo += ":";
		memset(buf, 0, sizeof(buf));
		sprintf_s(buf, "%d", 0);
		strInfo += buf;

		strInfo += ",";
		memset(buf, 0, sizeof(buf));
		sprintf_s(buf, "%d", 0);
		strInfo += buf;

		strInfo += "\"}";

		strInfo = ANSItoUTF8(strInfo);
		if (m_pLiveManager != NULL)
		{
			m_pLiveManager->insertRealtimeData((uint8_t*)strInfo.c_str(), strInfo.length());
		}
	}
}

void CMinClassDlg::OnBnClickedButtonMinClassApplyUpload()
{
	m_pLiveManager->applyToBroadcaster(m_pCurrentLive->m_strCreator.GetBuffer(0));
}


void CMinClassDlg::OnBnClickedButtonMinClassExitUpload()
{
	stopMinClass();

	bool bRet = watchMinClass();
	if(!bRet)
	{
		AfxMessageBox("加入小班课失败！");
		PostMessage(WM_CLOSE, 0, 0);
	}
	GetDlgItem(IDC_BUTTON_MIN_CLASS_APPLY_UPLOAD)->EnableWindow(true);
	GetDlgItem(IDC_BUTTON_MIN_CLASS_EXIT_UPLOAD)->EnableWindow(false);

	GetDlgItem(IDC_BUTTON_MIN_CLASS_STYLE)->EnableWindow(!m_bWatch);
	GetDlgItem(IDC_BUTTON_MIN_CLASS_SET_COLOR)->EnableWindow(!m_bWatch);
	GetDlgItem(IDC_BUTTON_MIN_CLASS_REVOKE)->EnableWindow(!m_bWatch);
	GetDlgItem(IDC_BUTTON_MIN_CLASS_CLEAR)->EnableWindow(!m_bWatch);
}

LRESULT CMinClassDlg::OnRecvMinClassMsg(WPARAM wParam, LPARAM lParam)
{
	switch (wParam)
	{
	case 0:
		break;
	case 1:
	{
		if (IDYES == AfxMessageBox("是否上传数据", MB_YESNO))
		{
			stopMinClass();
			bool bRet = startMinClass();
			//m_ApplyUploadButton.EnableWindow(FALSE);
			//m_ExitUpload.EnableWindow(FALSE);
			if (bRet)
			{
				GetDlgItem(IDC_BUTTON_MIN_CLASS_APPLY_UPLOAD)->EnableWindow(false);
				GetDlgItem(IDC_BUTTON_MIN_CLASS_EXIT_UPLOAD)->EnableWindow(true);
				
				GetDlgItem(IDC_BUTTON_MIN_CLASS_STYLE)->EnableWindow(true);
				GetDlgItem(IDC_BUTTON_MIN_CLASS_SET_COLOR)->EnableWindow(true);
				GetDlgItem(IDC_BUTTON_MIN_CLASS_REVOKE)->EnableWindow(true);
				GetDlgItem(IDC_BUTTON_MIN_CLASS_CLEAR)->EnableWindow(true);
			}
			else
			{
				AfxMessageBox("加入小班课失败！");
				PostMessage(WM_CLOSE, 0, 0);
			}
		}
		else
		{
		}
	}
	break;
	case 2:
		break;
	}
	return 0;
}

LRESULT CMinClassDlg::OnRecvWhitePanelMsg(WPARAM wParam, LPARAM lParam)
{
	CWhitePanelInfo* pWhitePanelInfo = (CWhitePanelInfo*)lParam;
	if (pWhitePanelInfo != NULL)
	{
		// android 数据格式{"account":"324137","data":"1:0.28557464,0.48759416,0"}
// {"account":"324137","data":"2:0.28557464,0.48759416,0;2:0.29121235,0.46772355,0;2:0.311973,0.40520847,0;2:0.32902429,0.3527772,0;2:0.34328133,0.3098482,0"}
// 激光笔：{"account":"324137","data":"12:0.92302036,0.683841,8781991;12:0.92302036,0.683841,8781991"}

		char buf[128] = { 0 };
		string strInfo = "{\"account\":\"";
		strInfo += m_pUserManager->m_ServiceParam.m_strUserId;
		strInfo += "\",\"data\":\"";
		sprintf_s(buf, "%d", pWhitePanelInfo->type);
		strInfo += buf;


		strInfo += ":";
		memset(buf, 0, sizeof(buf));
		sprintf_s(buf, "%f", pWhitePanelInfo->point.x);
		strInfo += buf;

		strInfo += ",";
		memset(buf, 0, sizeof(buf));
		sprintf_s(buf, "%f", pWhitePanelInfo->point.y);
		strInfo += buf;

		if (pWhitePanelInfo->type != WHITE_PANEL_LASER_PEN && pWhitePanelInfo->type != WHITE_PANEL_LASER_PEN_END)
		{
			strInfo += ",";
			memset(buf, 0, sizeof(buf));
			sprintf_s(buf, "%d", pWhitePanelInfo->lineColor);
			strInfo += buf;
		}
		strInfo += "\"}";

		strInfo = ANSItoUTF8(strInfo);
		if (m_pLiveManager != NULL)
		{
			m_pLiveManager->insertRealtimeData((uint8_t*)strInfo.c_str(), strInfo.length());
		}

		delete pWhitePanelInfo;
		pWhitePanelInfo = NULL;
	}
	return 0;
}

/**
 * 有主播加入
 * @param liveID 直播ID
 * @param actorID 新加入的主播ID
 */
void CMinClassDlg::onActorJoined(string liveID, string actorID)
{
	if (m_pDataShowView != NULL)
	{
		int nCount = m_pDataShowView->getUserCount();
		bool bBig = false;
		if (nCount == 0)
		{
			bBig = true;
		}
		m_pDataShowView->addUser(actorID, bBig);
		SetEvent(m_hSetShowPicEvent);
	}
}
/**
 * 有主播离开
 * @param liveID 直播ID
 * @param actorID 离开的主播ID
 */
void CMinClassDlg::onActorLeft(string liveID, string actorID)
{
	if (m_pDataShowView != NULL)
	{
		m_pDataShowView->removeUser(actorID);
		m_pLiveManager->changeToSmall(actorID);
		string strUserId = "";
		bool bLeft = m_pDataShowView->isLeftOneUser(strUserId);
		if (bLeft && strUserId != "")
		{
			m_pLiveManager->changeToBig(strUserId);
			m_pDataShowView->changeShowStyle(strUserId, true);
		}
		SetEvent(m_hSetShowPicEvent);
	}
}

/**
 * 房主收到其他用户的连麦申请
 * @param liveID
 * @param applyUserID
 */
void CMinClassDlg::onApplyBroadcast(string liveID, string applyUserID)
{
	CString strMsg;
	strMsg.Format("是否同意用户:%s申请连麦", applyUserID.c_str());
	if (m_pLiveManager != NULL)
	{
		if (IDYES == AfxMessageBox(strMsg, MB_YESNO))
		{
			m_pLiveManager->agreeApplyToBroadcaster(applyUserID);
			onActorJoined(liveID, applyUserID);
		}
		else
		{
			m_pLiveManager->refuseApplyToBroadcaster(applyUserID);
		}
	}
}

/**
 * 申请连麦用户收到的回复
 * @param liveID
 * @param result
 */
void CMinClassDlg::onApplyResponsed(string liveID, bool bAgree)
{
	if (bAgree)
	{
		PostMessage(WM_RECV_MIN_CLASS_MSG, 1, 0);
	}
	else
	{
		AfxMessageBox("对方拒绝连麦请求");
	}
}

/**
* 普通用户收到连麦邀请
* @param liveID 直播ID
* @param applyUserID 发出邀请的人的ID（主播ID）
*/
void CMinClassDlg::onInviteBroadcast(string liveID, string applyUserID)
{
}

/**
* 主播收到的邀请连麦结果
* @param liveID 直播ID
* @param result 邀请结果
*/
void CMinClassDlg::onInviteResponsed(string liveID)
{
}

/**
 * 一些异常情况引起的出错，请在收到该回调后主动断开直播
 * @param liveID 直播ID
 * @param error 错误信息
 */
void CMinClassDlg::onLiveError(string liveID, string error)
{
	//m_ApplyUploadButton.EnableWindow(FALSE);
	//m_ExitUpload.EnableWindow(FALSE);
	if (m_pDataShowView != NULL)
	{
		m_pDataShowView->removeAllUser();
		m_pDataShowView->setShowPictures();
	}
	stopGetData();
	if (m_pSoundManager != NULL)
	{
		m_pSoundManager->stopSoundData();
	}
	if (m_pCurrentLive != NULL)
	{
		delete m_pCurrentLive;
		m_pCurrentLive = NULL;
	}
}

/**
 * 聊天室成员数变化
 * @param number
 */
void CMinClassDlg::onMembersUpdated(int number)
{
}

/**
 * 自己被踢出聊天室
 */
void CMinClassDlg::onSelfKicked()
{
}

/**
 * 自己被踢出聊天室
 */
void CMinClassDlg::onSelfMuted(int seconds)
{
}

/**
* 连麦者的连麦被强制停止
* @param liveID 直播ID
*/
void CMinClassDlg::onCommandToStopPlay(string  liveID)
{
	//m_ApplyUploadButton.EnableWindow(FALSE);
	//m_ExitUpload.EnableWindow(FALSE);

	if (m_pDataShowView != NULL)
	{
		m_pDataShowView->removeAllUser();
		m_pDataShowView->setShowPictures();
	}
	stopGetData();
	if (m_pSoundManager != NULL)
	{
		m_pSoundManager->stopSoundData();
	}
	if (m_pLiveManager != NULL)
	{
		m_pLiveManager->leaveLive();
	}
	if (m_pCurrentLive != NULL)
	{
		delete m_pCurrentLive;
		m_pCurrentLive = NULL;
	}
}
/**
 * 收到消息
 * @param message
 */
void CMinClassDlg::onReceivedMessage(CIMMessage* pMessage)
{
	string str_json = pMessage->m_strContentData;
	Json::Reader reader;
	Json::Value root;
	if (str_json != "" && reader.parse(str_json, root))
	{
		string strVal = "";
		if (root.isMember("text"))
		{
			strVal = root["text"].asCString();
		}
		CString strMsg = "";
		strMsg.Format("%s:%s", pMessage->m_strFromId.c_str(), strVal.c_str());
		m_HistoryMsg.InsertString(m_HistoryMsg.GetCount(), strMsg);
	}	
}

/**
 * 收到私信消息
 * @param message
 */
void CMinClassDlg::onReceivePrivateMessage(CIMMessage* pMessage)
{
	string str_json = pMessage->m_strContentData;
	Json::Reader reader;
	Json::Value root;
	if (str_json != "" && reader.parse(str_json, root))
	{
		string strVal = "";
		if (root.isMember("text"))
		{
			strVal = root["text"].asCString();
		}
		CString strMsg = "";
		strMsg.Format("%s:%s", pMessage->m_strFromId.c_str(), strVal.c_str());
		m_HistoryMsg.InsertString(m_HistoryMsg.GetCount(), strMsg);
	}
}


int CMinClassDlg::getRealtimeData(string strUserId, uint8_t* data, int len)
{
	string strData = (char*)data;
	strData = UTF8toANSI(strData);
	// android 数据格式{"account":"324137","data":"1:0.28557464,0.48759416,0"}
// {"account":"324137","data":"2:0.28557464,0.48759416,0;2:0.29121235,0.46772355,0;2:0.311973,0.40520847,0;2:0.32902429,0.3527772,0;2:0.34328133,0.3098482,0"}
// 激光笔：{"account":"324137","data":"12:0.92302036,0.683841,8781991;12:0.92302036,0.683841,8781991"}

	string str_json = strData;
	Json::Reader reader;
	Json::Value root;
	if (str_json != "" && reader.parse(str_json, root))  // reader将Json字符串解析到root，root将包含Json里所有子元素   
	{
		string strAccount = "";
		if (root.isMember("account"))
		{
			strAccount = root["account"].asCString();
		}
		vector<CWhitePanelInfo> whitePanelInfo;
		if (root.isMember("data"))
		{
			string strVal = root["data"].asCString();
			int pos = strVal.find(";");
			while (pos > 0 || strVal.length() > 0)
			{
				string str = "";
				if (pos > 0)
				{
					str = strVal.substr(0, pos);
					strVal = strVal.substr(pos + 1, strVal.length() - pos - 1);
				}
				else
				{
					str = strVal;
					strVal = "";
				}
				int nPos1 = str.find(":");
				if (nPos1 > 0)
				{
					CWhitePanelInfo info;
					string strType = str.substr(0, nPos1);
					int nType = atoi(strType.c_str());
					info.type = (WHITE_PANEL_ACTION)nType;

					str = str.substr(nPos1 + 1, str.length() - nPos1 - 1);
					int nPos2 = str.find(",");
					if (nPos2 > 0)
					{
						string strX = str.substr(0, nPos2);
						double dx = atof(strX.c_str());
						str = str.substr(nPos2 + 1, str.length() - nPos2 - 1);
						nPos2 = str.find(",");
						if (nPos2 > 0 || str.length()>0)
						{
							string strY = "";
							if (nPos2 > 0)
							{
								strY = str.substr(0, nPos2);
								str = str.substr(nPos2 + 1, str.length() - nPos2 - 1);
							}
							else
							{
								strY = str;
								str = "";
							}
							double dy = atof(strY.c_str());
							int nColor = 0;
							if (str.length() > 0)
							{
								nColor = atoi(str.c_str());
							}
							
							info.point.x = dx;
							info.point.y = dy;
							info.lineColor = nColor;
							whitePanelInfo.push_back(info);
						}
					}
				}
				pos = strVal.find(";");
			}
		}

		if (strAccount != "" && whitePanelInfo.size() > 0)
		{
			m_WhitePanelDlg.setAction(strAccount, whitePanelInfo);
		}	
	}
	return 0;
}
int CMinClassDlg::getVideoRaw(string strUserId, int w, int h, uint8_t* videoData, int videoDataLen)
{
	if (m_pDataShowView != NULL)
	{
		m_pDataShowView->drawPic(FMT_YUV420P, strUserId, w, h, videoData, videoDataLen);
	}
	return 0;
}
void CMinClassDlg::addUpId()
{
	onActorJoined(m_pCurrentLive->m_strId.GetBuffer(0), m_pUserManager->m_ServiceParam.m_strUserId);
}
void CMinClassDlg::insertVideoRaw(uint8_t* videoData, int dataLen, int isBig)
{
	if (m_pLiveManager != NULL)
	{
		m_pLiveManager->insertVideoRaw(videoData, dataLen, isBig);
	}
}

int CMinClassDlg::cropVideoRawNV12(int w, int h, uint8_t* videoData, int dataLen, int yuvProcessPlan, int rotation, int needMirror, uint8_t* outVideoDataBig, uint8_t* outVideoDataSmall)
{
	int ret = 0;
	if (m_pLiveManager != NULL)
	{
		ret = m_pLiveManager->cropVideoRawNV12(w, h, videoData, dataLen, yuvProcessPlan, 0, 0, outVideoDataBig, outVideoDataSmall);
	}
	return ret;
}
void CMinClassDlg::drawPic(YUV_TYPE type, int w, int h, uint8_t* videoData, int videoDataLen)
{
	if (m_pDataShowView != NULL)
	{
		m_pDataShowView->drawPic(type, m_pUserManager->m_ServiceParam.m_strUserId, w, h, videoData, videoDataLen);
	}
}
void CMinClassDlg::getLocalSoundData(char* pData, int nLength)
{
	if (m_pLiveManager != NULL)
	{
		m_pLiveManager->insertAudioRaw((uint8_t*)pData, nLength);
	}
}

void CMinClassDlg::querySoundData(char** pData, int* nLength)
{
	if (m_pLiveManager != NULL)
	{
		m_pLiveManager->querySoundData((uint8_t**)pData, nLength);
	}
}
void CMinClassDlg::actionCallback(WHITE_PANEL_ACTION type, CScreenPoint& point, int lineColor)
{
	CWhitePanelInfo* pWhitePanelInfo = new CWhitePanelInfo();
	pWhitePanelInfo->type = type;
	pWhitePanelInfo->point.x = point.x;
	pWhitePanelInfo->point.y = point.y;
	pWhitePanelInfo->lineColor = lineColor;
	PostMessage(WM_RECV_WHITE_PANEL_MSG, 1, (LPARAM)pWhitePanelInfo);

}
void CMinClassDlg::stopMinClass()
{
	//m_ApplyUploadButton.EnableWindow(FALSE);
	//m_ExitUpload.EnableWindow(FALSE);
	if (m_pDataShowView != NULL)
	{
		m_pDataShowView->removeAllUser();
		m_pDataShowView->setShowPictures();
	}
	stopGetData();
	if (m_pSoundManager != NULL)
	{
		m_pSoundManager->stopSoundData();
	}
}

bool CMinClassDlg::watchMinClass()
{
	bool bRet = m_pLiveManager->watchLive(m_pCurrentLive->m_strId.GetBuffer());
	if (bRet)
	{
		startGetData((CROP_TYPE)m_pUserManager->m_ServiceParam.m_CropType, false);
		if (m_pSoundManager != NULL)
		{
			m_pSoundManager->startSoundData(false);
		}
		m_WhitePanelDlg.isGetData(false);
	}
	return bRet;
}

bool CMinClassDlg::startMinClass()
{
	bool bRet = m_pLiveManager->startLive(m_pCurrentLive->m_strId.GetBuffer());
	if (bRet)
	{
		startGetData((CROP_TYPE)m_pUserManager->m_ServiceParam.m_CropType, true);
		if (m_pSoundManager != NULL)
		{
			m_pSoundManager->startSoundData(true);
		}
		m_WhitePanelDlg.isGetData(true);
	}
	return bRet;
}

bool CMinClassDlg::createMinClass(string strName)
{
	bool bRet = false;
	XH_CHATROOM_TYPE chatRoomType = XH_CHATROOM_TYPE::XH_CHATROOM_TYPE_GLOBAL_PUBLIC;
	XH_LIVE_TYPE channelType = XH_LIVE_TYPE::XH_LIVE_TYPE_GLOBAL_PUBLIC;

	if (m_pLiveManager != NULL)
	{
		string strLiveId = m_pLiveManager->createLive(strName, chatRoomType, channelType);
		if (strLiveId != "")
		{
			string strInfo = "{\"id\":\"";
			strInfo += strLiveId;
			strInfo += "\",\"creator\":\"";
			strInfo += m_pUserManager->m_ServiceParam.m_strUserId;
			strInfo += "\",\"name\":\"";
			strInfo += strName;
			strInfo += "\"}";
			if (m_pConfig != NULL && m_pConfig->m_bAEventCenterEnable)
			{
				CInterfaceUrls::demoSaveToList(m_pUserManager->m_ServiceParam.m_strUserId, CHATROOM_LIST_TYPE_CLASS, strLiveId, strInfo);
			}
			else
			{
				m_pLiveManager->saveToList(m_pUserManager->m_ServiceParam.m_strUserId, CHATROOM_LIST_TYPE_CLASS, strLiveId, strInfo);
			}

			bRet = true;
			if (m_pCurrentLive == NULL)
			{
				m_pCurrentLive = new CLiveProgram();
			}
			m_pCurrentLive->m_strId = strLiveId.c_str();
			m_pCurrentLive->m_strName = strName.c_str();
			m_pCurrentLive->m_strCreator = m_pUserManager->m_ServiceParam.m_strUserId.c_str();
		}
	}
	return bRet;
}

void CMinClassDlg::setConfig(CConfigManager* pConfig)
{
	m_pConfig = pConfig;
}

void CMinClassDlg::OnMove(int x, int y)
{
	__super::OnMove(x, y);

	CRect whitePanelArea;
	::GetWindowRect(m_WhitePanelArea.m_hWnd, whitePanelArea);
	m_WhitePanelDlg.MoveWindow(whitePanelArea, true);
	m_WhitePanelDlg.ShowWindow(SW_SHOW);
}



