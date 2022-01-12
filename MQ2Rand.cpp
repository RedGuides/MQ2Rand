/********************************************************************************
****
**      MQ2Rand
**
**      Author: Dewey2461@gmail.com
**
**      This plugin helps a raid leader perform random loot sort and announcements
**
****
*********************************************************************************/

#include <mq/Plugin.h>

PreSetup("MQ2Rand");
PLUGIN_VERSION(1.0);

#define TIME unsigned long long

int  ErrorLoadingXML = 0;
int  SortListBy = 0;
TIME ListUpdatedTM = 0;
TIME MyTick = 1;

int  Btn1Pressed = 0;
int  Btn2Pressed = 0;
int  Btn3Pressed = 0;

int  RowSelected = 0;
TIME RowSelectedTM = 0;
int  PluginON = 1;
int  MyWindowState = 0;
int  WindowLoaded = 0;
int  SortNeeded = 0;
int  MaxRand = 999;
int  Active = 0;

char szSection[MAX_STRING] = "None";
char szOutput[MAX_STRING] = "None";

typedef struct _trROLL
{
	char Name[40];
	int  Roll;                      // Use ID's instead of pointers to reference this node.
	struct _trROLL* pNext;
} trROLL;

trROLL* pList = NULL;

int CompNode(trROLL* p1, trROLL* p2)
{
	int v = 0;
	if (SortListBy < 2) v = p1->Roll - p2->Roll;
	if (v) return v;
	return strcmp(p2->Name, p1->Name);
}

trROLL* GetNodeByName(char* Name)
{
	trROLL* p = pList;
	while (p) {
		if (strcmp(Name, p->Name) == 0) {
			return p;
		}
		p = p->pNext;
	}
	return p;
}


trROLL* AddNewNode(char* Name, int Roll)
{
	trROLL* p;
	p = (trROLL*)malloc(sizeof(trROLL));
	memset(p, 0, sizeof(trROLL));

	// just drop it in front and resort later
	p->pNext = pList;
	pList = p;

	strncpy_s(p->Name, Name, 40);
	p->Name[39] = 0;
	p->Roll = Roll;
	ListUpdatedTM = 1;
	SortNeeded = 1;
	return p;
}

void SortList()
{
	if (!pList) return;
	//WriteChatf("SortList");

	trROLL* pSrc = pList;
	trROLL* p, * q, * n;

	pList = NULL;
	while (pSrc)
	{
		// remove the node
		n = pSrc;
		pSrc = pSrc->pNext;

		// traverse the list looking for where it fits
		p = pList;
		q = NULL;
		while (p && CompNode(p, n) > 0) {
			q = p;
			p = p->pNext;
		}

		// insert it
		n->pNext = p;
		if (q)
			q->pNext = n;
		else
			pList = n;
	}
	SortNeeded = 0;
	ListUpdatedTM = MyTick;
}

void DestroyList()
{
	trROLL* p = pList;
	trROLL* q;
	while (p)
	{
		q = p->pNext;
		free(p);
		p = q;
	}
	pList = NULL;
}

void ReadWindowINI(CSidlScreenWnd* pWindow);
void WriteWindowINI(CSidlScreenWnd* pWindow);


class CMyWnd : public CCustomWnd
{
public:

	CListWnd* List;
	CButtonWnd* Btn1;
	CButtonWnd* Btn2;
	CButtonWnd* Btn3;
	CEditWnd* Edit;
	CLabelWnd* Label;

	CMyWnd() :CCustomWnd("RandWnd") {
		ErrorLoadingXML = 0;
		List = (CListWnd*)GetChildItem("RW_List");
		Btn1 = (CButtonWnd*)GetChildItem("RW_Btn1");
		Btn2 = (CButtonWnd*)GetChildItem("RW_Btn2");
		Btn3 = (CButtonWnd*)GetChildItem("RW_Btn3");
		Label = (CLabelWnd*)GetChildItem("RW_Label");
		Edit = (CEditWnd*)GetChildItem("RW_Edit");
		if (!List || !Btn1 || !Btn2 || !Btn3 || !Label || !Edit) {
			ErrorLoadingXML = 1;
			WriteChatf("\agMQ2Rand\ax \ayMQUI_RandWnd.XML\ax - Missing UI element - Get new UI file");
			return;
		}
	}
	void ShowWin() { ((CXWnd*)this)->Show(1, 1); ((CXWnd*)this)->SetVisible(1); }
	void HideWin() { ((CXWnd*)this)->Show(0, 0); ((CXWnd*)this)->SetVisible(0); }

	int  WndNotification(CXWnd* pWnd, unsigned int Message, void* pVoid) {
		if (ErrorLoadingXML) return 0;

		if (Message == XWM_COLUMNCLICK && pWnd == (CXWnd*)List) {
			int C = (int)(pVoid);
			SortListBy = C;
			ListUpdatedTM = MyTick;
			SortNeeded = 1;

			//WriteChatf("Messaage::Column = %d ",C);
		}
		// Left mouse up ?
		if (Message == XWM_LCLICK) {
			if (pWnd == (CXWnd*)List) {
				RowSelected = (int)(pVoid);
				RowSelectedTM = MyTick;
			}
			if (pWnd == (CXWnd*)Btn1) Btn1Pressed = 1;
			if (pWnd == (CXWnd*)Btn2) Btn2Pressed = 1;
			if (pWnd == (CXWnd*)Btn3) Btn3Pressed = 1;
		}

		return CSidlScreenWnd::WndNotification(pWnd, Message, pVoid);
	};
};

CMyWnd* MyWnd = 0;

void CreateMyWindow()
{
	if (MyWnd) return;
	if (pSidlMgr->FindScreenPieceTemplate("RandWnd")) {
		MyWnd = new CMyWnd;
		if (MyWnd && ErrorLoadingXML) {
			delete MyWnd;
			MyWnd = 0;
		}
		if (MyWnd) {
			ReadWindowINI(MyWnd);
			WriteWindowINI(MyWnd);
			if (MyWnd->IsMinimized()) MyWnd->SetMinimized(0);
			ListUpdatedTM = MyTick;
		}
	}
}

void DestroyMyWindow()
{
	if (MyWnd)
	{
		WriteWindowINI(MyWnd);
		MyWnd->List->DeleteAll();
		delete MyWnd;
		MyWnd = 0;
	}

}

void GetSectionName()
{
	if (!GetCharInfo()) return;
	sprintf_s(szSection, "%s.%s", EQADDR_SERVERNAME, GetCharInfo()->Name);
}

template <unsigned int _Size>LPSTR SafeItoa(int _Value, char(&_Buffer)[_Size], int _Radix)
{
	errno_t err = _itoa_s(_Value, _Buffer, _Radix);
	if (!err) {
		return _Buffer;
	}
	return "";
}


void ReadINI()
{
	GetSectionName();
	CHAR szTemp[MAX_STRING] = { 0 };
	PluginON = GetPrivateProfileInt(szSection, "ON", 1, INIFileName);
}

void WriteINI()
{
	GetSectionName();
	CHAR szTemp[MAX_STRING] = { 0 };

	WritePrivateProfileString(szSection, "ON", SafeItoa(PluginON, szTemp, 10), INIFileName);
}

void ReadWindowINI(CSidlScreenWnd* pWindow)
{
	char Buffer[MAX_STRING] = { 0 };
	GetSectionName();

	LONG L = GetPrivateProfileInt(szSection, "ChatLeft", 10, INIFileName);
	LONG T = GetPrivateProfileInt(szSection, "ChatTop", 10, INIFileName);

	pWindow->SetLocation({ L,T,L + 200,T + 12 });

	pWindow->SetEscapable(0);

	pWindow->SetLocked((GetPrivateProfileInt(szSection, "Locked", 0, INIFileName) ? true : false));
	pWindow->SetFades((GetPrivateProfileInt(szSection, "Fades", 1, INIFileName) ? true : false));
	pWindow->SetFadeDelay(GetPrivateProfileInt(szSection, "Delay", 2000, INIFileName));
	pWindow->SetFadeDuration(GetPrivateProfileInt(szSection, "Duration", 500, INIFileName));
	pWindow->SetAlpha(GetPrivateProfileInt(szSection, "Alpha", 255, INIFileName));
	pWindow->SetFadeToAlpha(GetPrivateProfileInt(szSection, "FadeToAlpha", 255, INIFileName));
	pWindow->SetBGType(GetPrivateProfileInt(szSection, "BGType", 1, INIFileName));
	ARGBCOLOR col = { 0 };
	col.A = GetPrivateProfileInt(szSection, "BGTint.alpha", 255, INIFileName);
	col.R = GetPrivateProfileInt(szSection, "BGTint.red", 0, INIFileName);
	col.G = GetPrivateProfileInt(szSection, "BGTint.green", 0, INIFileName);
	col.B = GetPrivateProfileInt(szSection, "BGTint.blue", 0, INIFileName);
	pWindow->SetBGColor(col.ARGB);
}

void WriteWindowINI(CSidlScreenWnd* pWindow)
{
	CHAR szTemp[MAX_STRING] = { 0 };
	GetSectionName();

	if (pWindow->IsMinimized())
	{
		WritePrivateProfileString(szSection, "ChatTop", SafeItoa(pWindow->GetOldLocation().top, szTemp, 10), INIFileName);
		WritePrivateProfileString(szSection, "ChatBottom", SafeItoa(pWindow->GetOldLocation().bottom, szTemp, 10), INIFileName);
		WritePrivateProfileString(szSection, "ChatLeft", SafeItoa(pWindow->GetOldLocation().left, szTemp, 10), INIFileName);
		WritePrivateProfileString(szSection, "ChatRight", SafeItoa(pWindow->GetOldLocation().right, szTemp, 10), INIFileName);
	}
	else
	{
		WritePrivateProfileString(szSection, "ChatTop", SafeItoa(pWindow->GetLocation().top, szTemp, 10), INIFileName);
		WritePrivateProfileString(szSection, "ChatBottom", SafeItoa(pWindow->GetLocation().bottom, szTemp, 10), INIFileName);
		WritePrivateProfileString(szSection, "ChatLeft", SafeItoa(pWindow->GetLocation().left, szTemp, 10), INIFileName);
		WritePrivateProfileString(szSection, "ChatRight", SafeItoa(pWindow->GetLocation().right, szTemp, 10), INIFileName);
	}
	WritePrivateProfileString(szSection, "Locked", SafeItoa(pWindow->IsLocked(), szTemp, 10), INIFileName);
	WritePrivateProfileString(szSection, "Fades", SafeItoa(pWindow->GetFades(), szTemp, 10), INIFileName);
	WritePrivateProfileString(szSection, "Delay", SafeItoa(pWindow->GetFadeDelay(), szTemp, 10), INIFileName);
	WritePrivateProfileString(szSection, "Duration", SafeItoa(pWindow->GetFadeDuration(), szTemp, 10), INIFileName);
	WritePrivateProfileString(szSection, "Alpha", SafeItoa(pWindow->GetAlpha(), szTemp, 10), INIFileName);
	WritePrivateProfileString(szSection, "FadeToAlpha", SafeItoa(pWindow->GetFadeToAlpha(), szTemp, 10), INIFileName);
	WritePrivateProfileString(szSection, "BGType", SafeItoa(pWindow->GetBGType(), szTemp, 10), INIFileName);
	ARGBCOLOR col = { 0 };
	col.ARGB = pWindow->GetBGColor();
	WritePrivateProfileString(szSection, "BGTint.alpha", SafeItoa(col.A, szTemp, 10), INIFileName);
	WritePrivateProfileString(szSection, "BGTint.red", SafeItoa(col.R, szTemp, 10), INIFileName);
	WritePrivateProfileString(szSection, "BGTint.green", SafeItoa(col.G, szTemp, 10), INIFileName);
	WritePrivateProfileString(szSection, "BGTint.blue", SafeItoa(col.B, szTemp, 10), INIFileName);
}

void WindowSetSizeLoc(CXWnd* pWindow, long X, long Y, long DX, long DY)
{
	pWindow->Move({ X, Y, X + DX, Y + DY });
}

void WindowSetSize(CXWnd* pWindow, long DX, long DY)
{
	long T = pWindow->GetLocation().top;
	long B = pWindow->GetLocation().bottom;
	long L = pWindow->GetLocation().left;
	long R = pWindow->GetLocation().right;

	if (DX == -1) DX = R - L;
	if (DY == -1) DY = B - T;

	pWindow->Move({ L, T, L + DX, T + DY });
}

void WindowSetLoc(CXWnd* pWindow, long X, long Y)
{
	long T = pWindow->GetLocation().top;
	long B = pWindow->GetLocation().bottom;
	long L = pWindow->GetLocation().left;
	long R = pWindow->GetLocation().right;

	long DX = R - L;
	long DY = B - T;

	pWindow->Move({ X, Y, X + DX, Y + DY });
}

void SetupButton(CXWnd* pWindow, int Vis, long X, long Y, long DX, long DY, char* zFormat, ...)
{
	pWindow->SetVisible(Vis);
	if (!Vis) return;
	if (X && Y && DX && DY)
		WindowSetSizeLoc(pWindow, X, Y, DX, DY);
	else if (X && Y)
		WindowSetLoc(pWindow, X, Y);
	else if (DX && DY)
		WindowSetSize(pWindow, DX, DY);

	if (zFormat && zFormat[0]) {
		va_list vaList; va_start(vaList, zFormat);
		vsprintf_s(szOutput, zFormat, vaList);
		if (szOutput[0])
			pWindow->SetWindowText(szOutput);
	}
}


void SetupColumn(CListWnd* pList, int column, int justification, int width, char* zFormat, ...)
{
	if (!pList) return;
	if (zFormat && zFormat[0]) {
		va_list vaList; va_start(vaList, zFormat);
		vsprintf_s(szOutput, zFormat, vaList);
		if (szOutput[0])
			pList->SetColumnLabel(column, CXStr(szOutput));
	}
	if (width >= 0) pList->SetColumnWidth(column, width);
	pList->SetColumnJustification(column, justification);
}

void SetListRCText(CListWnd* pList, int row, int column, size_t max, char* zFormat, ...)
{
	if (!pList) return;
	if (zFormat && zFormat[0]) {
		va_list vaList; va_start(vaList, zFormat);
		vsprintf_s(szOutput, zFormat, vaList);
		if (strlen(szOutput) > max) szOutput[max] = 0;
		if (szOutput[0])
			pList->SetItemText(row, column, szOutput);
	}
}



void HideMyWindow()
{
	if (!MyWnd) return;
	MyWnd->HideWin();
	WriteINI();
}

void ShowMyWindow()
{
	if (gGameState != GAMESTATE_INGAME || !pCharSpawn) return;
	WindowLoaded = 1;
	if (!PluginON) {
		WriteChatf("\ag[MQ2Rand]\aw Unable to show window while plugin is OFF");
		return;
	}
	if (!MyWnd) CreateMyWindow();
	if (!MyWnd) {
		WriteChatf("MQ2Rand: Failed to open window.");
		WriteChatf("MQ2Rand: MQUI_RandWnd.XML old or not found.");
		WriteChatf("MQ2Rand: This is normal when you first load plugin");
		WriteChatf("MQ2Rand: /camp desktop and log back in");
		ErrorLoadingXML = 1;
		return;
	}
	MyWnd->ShowWin();
	WriteINI();
}



fEQCommand  pEQRandFunc = NULL;


void GetOrginalRandomCommand()
{
	PCMDLIST pCmdListOrig = (PCMDLIST)EQADDR_CMDLIST;
	for (int i = 0; pCmdListOrig[i].fAddress != 0; i++) {
		if (!strcmp(pCmdListOrig[i].szName, "/random")) {
			pEQRandFunc = (fEQCommand)pCmdListOrig[i].fAddress;
		}
	}
}


void Command(PSPAWNINFO pChar, PCHAR Cmd)
{
	char Arg1[MAX_STRING];
	char Arg2[MAX_STRING];

	GetArg(Arg1, Cmd, 1);
	GetArg(Arg2, Cmd, 2);

	if (_stricmp(Arg1, "show") == 0) { ShowMyWindow(); return; }
	if (_stricmp(Arg1, "hide") == 0) { HideMyWindow(); return; }

	if (pEQRandFunc) pEQRandFunc(pChar, Cmd);
}



// Called once, when the plugin is to initialize
PLUGIN_API void InitializePlugin()
{
	GetOrginalRandomCommand();
	AddXMLFile("MQUI_RandWnd.xml");
	AddCommand("/random", Command);
}

// Called once, when the plugin is to shutdown
PLUGIN_API void ShutdownPlugin()
{
	RemoveCommand("/random");
	DestroyMyWindow();
	RemoveXMLFile("MQUI_RandWnd.xml");
	DestroyList();
}

PLUGIN_API void OnCleanUI()
{
	DestroyMyWindow();
}

PLUGIN_API void OnReloadUI()
{
	if (gGameState == GAMESTATE_INGAME && pCharSpawn)
	{
		if (MyWindowState == 1) ShowMyWindow();
	}
}

PLUGIN_API void SetGameState(int GameState) {
	if (GameState == GAMESTATE_INGAME && GetCharInfo()) {
		ReadINI();
		if (!PluginON) return;
		//ShowMyWindow();
	}
}

PLUGIN_API void OnPulse()
{
	MyTick = MQGetTickCount64();

	static TIME TM = 0;
	if (rand() % 100 > 95) {
		//WriteChatf("OnPulse::Active %d",Active);

	}

	if (gGameState != GAMESTATE_INGAME) return;
	if (!pCharSpawn) return;
	if (Active > 0 && !WindowLoaded) ShowMyWindow();
	if (!MyWnd) return;


	if (ListUpdatedTM > TM) {
		SetupButton((CXWnd*)MyWnd->Edit, 1, 5, 1, 65, 20, " ");

		MyWnd->Label->SetVisible(0);
		MyWnd->List->SetVisible(0);

		sprintf_s(szOutput, "%d", MaxRand);
		MyWnd->Edit->InputText = szOutput;


		MyWnd->List->SetVisible(1);
		SetupColumn(MyWnd->List, 0, 1, 35, "#");
		SetupColumn(MyWnd->List, 1, 0, 45, "Val");
		SetupColumn(MyWnd->List, 2, 0, 160, "Player");

		WindowSetLoc(MyWnd->List, 1, 51);
		WindowSetSize(MyWnd->List, 170, 22 + 14 * 10);
		WindowSetSize(MyWnd, 170, 104 + 14 * 10);

		if (SortNeeded) SortList();
		ListUpdatedTM = MyTick;

		MyWnd->List->DeleteAll();

		int N = 0;
		trROLL* p = pList;
		while (p) {

			MyWnd->List->AddString("", 0xFFFFFFFF, 0, 0);

			SetListRCText(MyWnd->List, N, 0, 25, "%d", N + 1);
			SetListRCText(MyWnd->List, N, 1, 25, "%d", p->Roll);
			SetListRCText(MyWnd->List, N, 2, 125, "%s", p->Name);

			p = p->pNext;
			N++;
		}

		TM = MyTick;
	}


	if (Btn1Pressed) {
		if (Active == 0) {
			MaxRand = GetIntFromString(MyWnd->Edit->InputText, 0);
			if (MaxRand > 0) {
				DestroyList();
				ListUpdatedTM = MyTick;
				sprintf_s(szOutput, "/rs Start RAND %d ", MaxRand);
				EzCommand(szOutput);
				Active = 1;
			}
		}

		if (Active == 2) {
			EzCommand("/rs RAND Unlocked");
			ListUpdatedTM = MyTick;
			Active = 1;
		}
	}

	if (Btn2Pressed == 1 && Active == 1) {
		Active = 2;
		ListUpdatedTM = MyTick;
		EzCommand("/rs RAND LOCKED");
	}

	if (Btn3Pressed == 1) {
		DestroyList();
		ListUpdatedTM = MyTick;
		Active = 0;

	}


	if (RowSelected >= 0 && Active == 2) {
		trROLL* p = pList;
		int N = 0;
		while (p && N != RowSelected) {
			p = p->pNext;
			N++;
		}
		if (p) {
			sprintf_s(szOutput, "/rs RAND #%d  %d  %s", N + 1, p->Roll, p->Name);
			EzCommand(szOutput);
		}
	}


	SetupButton(MyWnd->Btn1, 1, 75, 1, 65, 20, "Start");
	if (Active == 1) SetupButton(MyWnd->Btn1, 1, 75, 1, 65, 20, "ACTIVE");
	if (Active == 2) SetupButton(MyWnd->Btn1, 1, 75, 1, 65, 20, "Unlock");

	SetupButton(MyWnd->Btn2, 1, 75, 27, 65, 20, "Lock");
	if (Active == 2) SetupButton(MyWnd->Btn2, 1, 75, 27, 65, 20, "LOCKED");

	SetupButton(MyWnd->Btn3, 1, 5, 27, 65, 20, "Clear");


	Btn1Pressed = Btn2Pressed = Btn3Pressed = 0;
	RowSelected = -1;
}

PLUGIN_API int OnIncomingChat(const char* Line, int Color)
{
	//if (Line[0]!='-') WriteChatf("-- %s -- %d --", Line, Color);

	//if (Color == 287 || Color == 360) {

	if (strncmp(Line, "**A Magic Die is rolled by", 25) == 0) {
		const char* p1 = strstr(Line, " from ");
		const char* p2 = strstr(Line, " to ");
		const char* p3 = strstr(Line, " up a ");

		if (p1 && p2 && p3) {
			int lo, hi, v;
			lo = (int)atof(p1 + 6);
			hi = (int)atof(p2 + 4);
			v = (int)atof(p3 + 6);

			// Grab name
			char szName[40];
			const char* p = Line + 27;
			char* q = szName;
			int l = 0;
			while (*p != '.' && l < 35)
			{
				*q++ = *p++;
				l++;
			}
			*q = 0;

			//WriteChatf("lo:%d hi:%d v:%d w:%s", lo, hi, v,szName);



			if (Active == 1 && (hi != MaxRand || lo != 0)) {
				sprintf_s(szOutput, "/timed %d /rs RAND: [%s] roll invalid - reroll /rand %d", (rand() % 10) + 10, szName, MaxRand);
				EzCommand(szOutput);
				return 0;
			}

			if (Active == 2) {
				sprintf_s(szOutput, "/timed %d /rs RAND: [%s] rolled after rand locked.", (rand() % 10) + 10, szName);
				return 0;
			}


			if (hi == MaxRand && Active == 1) {
				trROLL* pNode = GetNodeByName(szName);
				if (pNode) {
					WriteChatf("MQ2Rand [%s] already rolled - skipping ", szName);
					return 0;
				}
				AddNewNode(szName, v);
				SortNeeded = 1;
				ListUpdatedTM = MyTick;
			}
		}
	}
	//}
	return 0;
}
