#include <windows.h>
#include <stdio.h>
#include <tlhelp32.h>
#include <time.h>
#include <conio.h>
#include "itemTimer.h"

bool SetUpWindowClass (char*, int, int, int);
LRESULT CALLBACK WindowProcedure (HWND, unsigned int, WPARAM, LPARAM);
void generateGameItems();


enum itemNames {
	Railgun = 6, 
	FiftyHealth = 42, 
	MegaHealth = 43,
	GreenArmor = 51,
	YellowArmor = 52,
	RedArmor = 53,
	QuadDamage = 60
};
//I was lazy so I used millions of globals. Sorry.
int g_itemIDs[] = {
	Railgun,
	FiftyHealth,
	MegaHealth,
	GreenArmor,
	YellowArmor,
	RedArmor,
	QuadDamage
};

struct gameItem {
	int address;
	byte itemID;
	bool isAvailable;
	bool isActive;

	char* itemName;
	COLORREF itemColor;
	COLORREF timerColor;
	itemTimer timer;
};

struct node {
	gameItem data;
	node* next;
};

HANDLE g_gameHandle; //this isn't a mistake, it needs to remain open for the entire runtime of the program

node* g_gameItemsRoot = NULL;
int g_baseAddress = -1;
int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpsCmdLine, int iCmdShow) {

	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(PROCESSENTRY32);

	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

	if (Process32First(snapshot, &entry) == TRUE)
	{
		while (Process32Next(snapshot, &entry) == TRUE)
		{
			if (_stricmp(entry.szExeFile, "reflex.exe") == 0)
			{  
				g_gameHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, entry.th32ProcessID);

				if(HANDLE hModuleSnap = CreateToolhelp32Snapshot( TH32CS_SNAPMODULE, entry.th32ProcessID )) { //index modules of target process
					MODULEENTRY32 me32;
					me32.dwSize = sizeof(MODULEENTRY32);
					if( !Module32First(hModuleSnap, &me32) ) { //get main module
						CloseHandle(hModuleSnap);
						exit(1); //couldn't find main module
					}
					g_baseAddress = (int)me32.modBaseAddr; //base address of main module is the base address of the entire program
				} else { exit(2); } //unable to index target process modules
			}
		}
	} else { exit(-1); } //couldn't find any process


	if (!SetUpWindowClass ("1", 0, 0, 0)) {
		exit(4); //error while registering window class
	}
	HWND hWnd = CreateWindowEx(WS_EX_TOPMOST, "1", "Game Info", NULL, 0, 0, 300, 300, NULL, NULL, hInstance, NULL);
	if (!hWnd) {
		exit(5); //window returned by CreateWindowEx() was NULL
	}
	SetWindowLong(hWnd, GWL_STYLE, 0); 
	ShowWindow (hWnd, SW_SHOW);
	MSG uMsg;
	generateGameItems(); //index all of the items so their states can be drawn
	SetTimer(hWnd, 0, 50, NULL); //create a timer event that elapses every 50ms (running the code under WM_TIMER in the winproc below)
	while (GetMessage (&uMsg, NULL, 0, 0) > 0) {
		TranslateMessage (&uMsg);
		DispatchMessage (&uMsg);
	}
	CloseHandle(g_gameHandle); //Window has stopped receiving messages. This should be unreachable but we'll clean up anyway
	return 0;
}

bool SetUpWindowClass (char* cpTitle, int iR, int iG, int iB) {
	WNDCLASSEX WindowClass;
	WindowClass.cbClsExtra = 0;
	WindowClass.cbWndExtra = 0;
	WindowClass.cbSize = sizeof (WNDCLASSEX);
	WindowClass.style = 0;
	WindowClass.lpszClassName = cpTitle;
	WindowClass.lpszMenuName = NULL;
	WindowClass.lpfnWndProc = WindowProcedure;
	WindowClass.hbrBackground = CreateSolidBrush (RGB(0, 0, 0));
	WindowClass.hInstance = GetModuleHandle (NULL);
	WindowClass.hCursor = LoadCursor (NULL, IDC_ARROW);
	WindowClass.hIcon = LoadIcon (NULL, IDI_APPLICATION);
	WindowClass.hIconSm = LoadIcon (NULL, IDI_APPLICATION);
	if (RegisterClassEx (&WindowClass)) return true;
	else return false;
}
/*---------------------------------------------------------------------
	WINDOW MESSAGE HANDLER
---------------------------------------------------------------------*/
LRESULT CALLBACK WindowProcedure (HWND hWnd, unsigned int uiMsg, WPARAM wParam, LPARAM lParam) {
	switch (uiMsg) {
	
	case WM_TIMER: {
		if(GetKeyState(VK_F4))
			generateGameItems();
		SIZE_T y;
		node* currentNode = g_gameItemsRoot;
		gameItem* currentItem;
		while(currentNode) { 
			currentItem = &(currentNode->data);
			ReadProcessMemory(g_gameHandle, (LPCVOID)(currentItem->address + 0x75), &(currentItem->isAvailable), sizeof(byte), &y);
			if((currentItem->isAvailable && currentItem->isActive) || (currentItem->timer.getTimeRemaining() < 0)) {
					currentItem->isActive = false;
				}
			if(!currentItem->isAvailable && !currentItem->isActive) {
				currentItem->timer.startTimer();
				currentItem->isActive = true;
			}
				
				currentNode = currentNode->next;
		} 
		InvalidateRect(hWnd, NULL, true); //redraw window
				   }
				   break;

	case WM_CLOSE:
		DestroyWindow (hWnd);
		break;
	case WM_DESTROY:
		CloseHandle(g_gameHandle);
		exit(6); //window closed by user
		/*---------------------------------------------------------------------
		  DRAW WINDOW
		---------------------------------------------------------------------*/
	case WM_PAINT: {
		PAINTSTRUCT ps;
		RECT rect;
		rect.left = 5;
		rect.top = 5;
		rect.bottom = 29;
		rect.right = 300;
		HDC hDC = BeginPaint (hWnd, &ps);
		HFONT font = CreateFont(24, 0, 0, 0, 300, false, false, false, 
			DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, 
			DEFAULT_QUALITY, DEFAULT_PITCH, "Arial");
		SelectObject( hDC, font );
		SetBkColor(hDC, RGB(0, 0, 0));
		gameItem currentItem;
		node* currentNode = g_gameItemsRoot;
		while(currentNode) {
			currentItem = currentNode->data;
			currentNode->data.timerColor = RGB(currentItem.timer.getTimeRemainingRatio()*255, 1-currentItem.timer.getTimeRemainingRatio()*255, 0);
			currentNode = currentNode->next;
		}

		currentNode = g_gameItemsRoot;
		while(currentNode) {
			currentItem = currentNode->data;
			if(currentItem.isActive && currentItem.timer.getTimeRemaining() > 0) {
				char itemName[50];
				sprintf_s(itemName, "%s: ", currentItem.itemName);
				SetTextColor(hDC, currentItem.itemColor);
				DrawText(hDC, itemName, -1, &rect, DT_NOCLIP);

				rect.left += 180;
				char timerText[20];
				sprintf_s(timerText, "%f\n", currentItem.timer.getTimeRemaining());
				SetTextColor(hDC, currentItem.timerColor);
				DrawText(hDC, timerText, -1, &rect, DT_NOCLIP);
				rect.left -= 180;
				rect.top += 24;
				rect.bottom += 24;
			}
			currentNode = currentNode->next;
		}
		EndPaint (hWnd, &ps);
				   }
				   break;
	}

	return DefWindowProc (hWnd, uiMsg, wParam, lParam);
}

void generateGameItems() {
	if(g_gameItemsRoot) { //destroy the existing tree
		node* currentNode = g_gameItemsRoot;
		node* previousNode;
		while(currentNode) {
			previousNode = currentNode;
			currentNode = currentNode->next;
			delete(previousNode);
		}
	}
	gameItem newItem;
	newItem.isActive = false;
	newItem.isAvailable = false;
	node* lastAdded = (node*)NULL;
	int currentPointer;
	int staticPtrAbsoluteAddress = 0x99DD7C + g_baseAddress; //static pointer to an array of pointers that point to game objects
	SIZE_T y;

	ReadProcessMemory(g_gameHandle, (LPCVOID)staticPtrAbsoluteAddress, &currentPointer, sizeof(int), &y); //now pointing at base of array of pointers
	bool listStarted = false;
	for(int i = 0; i < 1000; i++) {
		ReadProcessMemory(g_gameHandle, (LPCVOID)(currentPointer + (i * 0x4)), &(newItem.address), sizeof(int), &y); //physical address of the game item being handled
		ReadProcessMemory(g_gameHandle, (LPCVOID)(newItem.address + 0x74), &newItem.itemID, sizeof(byte), &y); //the internal ID of the item (see "itemNames")
		for(int j = 0; j < sizeof(g_itemIDs)/4; j++) { //check if itemID is recognized
			if(newItem.itemID == g_itemIDs[j]) { //if so, which one?
				switch(newItem.itemID) { //match it up to a name, color, and respawn time (i.e. armors are 25 as of the writing)
				case Railgun: newItem.itemName = "Railgun"; newItem.itemColor = RGB(255, 255, 255); newItem.timer.setMaxTime(10); break;
				case FiftyHealth: newItem.itemName = "50 Health"; newItem.itemColor = RGB(204,102,0); newItem.timer.setMaxTime(25); break;
				case MegaHealth: newItem.itemName = "Mega Health"; newItem.itemColor = RGB(51,255,255); newItem.timer.setMaxTime(30); break;
				case GreenArmor: newItem.itemName = "Green Armor"; newItem.itemColor = RGB(0,255,0); newItem.timer.setMaxTime(25); break;
				case YellowArmor: newItem.itemName = "Yellow Armor"; newItem.itemColor = RGB(255,255,0); newItem.timer.setMaxTime(25); break;
				case RedArmor: newItem.itemName = "Red Armor"; newItem.itemColor = RGB(255,0,0); newItem.timer.setMaxTime(25); break;
				case QuadDamage: newItem.itemName = "Quad Damage"; newItem.itemColor = RGB(204,0,204); newItem.timer.setMaxTime(90); break;
				default: newItem.itemName = "Unknown Item"; newItem.itemColor = RGB(255,255,255); newItem.timer.setMaxTime(9999); break;
				}//switch
				//add each item to a singly linked list
				if(!listStarted) {
					node* firstNode = new node;
					(*firstNode).data = newItem;
					(*firstNode).next = NULL;
					g_gameItemsRoot = firstNode;
					lastAdded = firstNode;
					listStarted = true;
				}
				else {
					node* newNode = new node;
					(*newNode).data = newItem;
					(*lastAdded).next = newNode;
					lastAdded = newNode;
					(*newNode).next = NULL;
				}
			}//if(is an item)
		}//for(itemIDs)
	}//for(find item bases)
}//function

