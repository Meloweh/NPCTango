#ifndef HEADER_H
#define HEADER_H
//jump definition spam for 1-14 byte jumps?
//#define jmp2(from, to) (int)(((int)to - (int)from) - 2);
//#define jmp3(from, to) (int)(((int)to - (int)from) - 3);
//#define jmp4(from, to) (int)(((int)to - (int)from) - 4);
#define jmp5(from, to) (int)(((int)to - (int)from) - 5);

#define _CRT_SECURE_NO_WARNINGS

#include <Windows.h>
#include <iostream>
#include <Psapi.h>
#include <sstream>

using namespace std;

char* module = "VisualBoyAdvance.exe";
unsigned short jumpLen = 5;

MODULEINFO GetModuleInfo(char* lenModule) {
	MODULEINFO modI = { 0 };
	HMODULE hmod = GetModuleHandle(lenModule);
	if (!hmod)
		return modI;

	GetModuleInformation(GetCurrentProcess(), hmod, &modI, sizeof(MODULEINFO));
	return modI;
}

DWORD aobscan(char *module, char *pattern, char *mask) {
	MODULEINFO modI = GetModuleInfo(module);
	DWORD base = (DWORD)modI.lpBaseOfDll,
		size = (DWORD)modI.SizeOfImage,
		patternLenght = (DWORD)strlen(mask);

	for (DWORD i = 0; i < size - patternLenght; i++) {
		bool found = true;
		//how it works:
		/*
		-> num1 num2 num3 num4
		ifn
		-> num2 num3 num4 num5
		ifn
		...
		*/
		for (DWORD j = 0; j < patternLenght; j++){
			found &= mask[j] == '?' || pattern[j] == *(char*)(base + i + j); // *(char) points at the memory location of our addy to access every single pattern byte combination
		}

		if (found)
			return base + i;
	}
	return 0;
}// *(char*) -> *(char) brings us to the memory address of our pattern and with (char*) we look at the exact byte at this position

void makeMemoryWriteable(ULONG uladdy, ULONG ulLen) {
	MEMORY_BASIC_INFORMATION *mbi = new MEMORY_BASIC_INFORMATION;
	VirtualQuery((void*)uladdy, mbi, ulLen);

	if (mbi->Protect != PAGE_EXECUTE_READWRITE) {
		ULONG *ulProtect = new ULONG;
		VirtualProtect((void*)uladdy, ulLen, PAGE_EXECUTE_READWRITE, ulProtect);
		delete ulProtect;
	}
	delete mbi;
}

bool Jump(ULONG uladdy, void *func, ULONG ulnops) { // jumpLen mostly 5
	__try {
		makeMemoryWriteable(uladdy, jumpLen + ulnops);
		*(UCHAR*)uladdy = 0xE9; //write jump byte

		*(ULONG*)(uladdy + 1) = jmp5(uladdy, func);			// write dword jump location to new memory

		memset((void*)(uladdy + jumpLen), 0x90, ulnops); //set some nops to prevent crash while overwriting

		return true;
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		MessageBoxA(NULL, "404 error.", "Jump failed", 0);
		return false;
	}
}

void writeBytes(DWORD addy, char arr[], unsigned int len){
	makeMemoryWriteable(addy, len);

	for (unsigned int i = 0; i <= len-1; i++) {
		memset((void*)(addy + i), arr[i], 1);
	}
}

void AddyMsg(DWORD addy)
{
	char lenBuf[1024];
	sprintf(lenBuf, "MemAddress: %02x", addy);
	MessageBox(0, lenBuf, "Memory address Location", MB_OK);
} 

//picked from https://stackoverflow.com/questions/6486289/how-can-i-clear-console
void clear() {
	COORD topLeft = { 0, 0 };
	HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO screen;
	DWORD written;

	GetConsoleScreenBufferInfo(console, &screen);
	FillConsoleOutputCharacterA(
		console, ' ', screen.dwSize.X * screen.dwSize.Y, topLeft, &written
		);
	FillConsoleOutputAttribute(
		console, FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE,
		screen.dwSize.X * screen.dwSize.Y, topLeft, &written
		);
	SetConsoleCursorPosition(console, topLeft);
}

//~~~~~~~~~~~ aob pattern definition ~~~~~~~~~~~//
char aobPositionBase[] = "\x0F\xB7\x04\x10\xE9\x96\x00\x00\x00\xA1\x5C\xF9\x72\x00\xA9\x00\x00\x00\xFF\x74\x16";
//char nopPositionBase[] = "\x90\x90\x90\x90";
char* maskAobPositionBase = "xxxxxxxxxxxxxxxxxxxxx";
const int nopcountPositionBase = 4;
DWORD dwAddyPositionBase = 0x0,
	  dwReturnPositionBase = 0x9F; // count bytes to the byte after aob addy and nops to return location
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

//------------ texture position base of the player ------------// 
DWORD dwTexturePositionBase = 0x20650;

//variables for values
DWORD dwPlayerTexturePos = 0xFFFFFFFF;
WORD wPlayerTextureX = 0xFFFF,
	 wPlayerTextureY = 0xFFFF;

//--------------------------------------- ------ -----   -// 

//------------ mask position base of the player ------------// 
DWORD dwMaskPositionBase = 0x37360;

//variables for values
DWORD dwPlayerMaskPos = 0xFFFFFFFF;
WORD wPlayerMaskX = 0xFFFF,
	 wPlayerMaskY = 0xFFFF;

DWORD arrPlayerPos[15];
DWORD arrPlayerPosX[15];
DWORD arrPlayerPosY[15];
//--------------------------------------- ------ -----// 

//------------ npc memory array specified variables ------------// 
DWORD dwNotUsedPositionMemory = 0x00A00130;//DIE BYTES SIND REVERSED //0x3001A000;
WORD bNPCCount = 0x0;
WORD lOffset = 0x0;
int iNPCIndex = 0;

void checkNewPlayerPosition(){
	if (arrPlayerPos[0] == 0x0) {
		arrPlayerPos[0] = dwPlayerTexturePos;
		arrPlayerPosX[0] = wPlayerTextureX;
		arrPlayerPosY[0] = wPlayerTextureY;
		return;
	}

	if (wPlayerTextureX <= arrPlayerPosX[0] - 0x10 ||
		wPlayerTextureX >= arrPlayerPosX[0] + 0x10 ||
		wPlayerTextureY <= arrPlayerPosY[0] - 0x10 ||
		wPlayerTextureY >= arrPlayerPosY[0] + 0x10) {

			for (int i = 14; i != 0; i--) {
				arrPlayerPos[i] = arrPlayerPos[i - 1];
				arrPlayerPosX[i] = arrPlayerPosX[i - 1];
				arrPlayerPosY[i] = arrPlayerPosY[i - 1];
			}
			arrPlayerPos[0] = dwPlayerTexturePos;
			arrPlayerPosX[0] = wPlayerTextureX;
			arrPlayerPosY[0] = wPlayerTextureY;
	}
}
//--------------------------------------- ----------------------// 

//~~~~~~~~~~~ adress calculation of aob patterns, into adresses ~~~~~~~~~~~//
bool calcAobs(){
	// texture and mask positions of player and NPCs
	dwAddyPositionBase = aobscan(module, aobPositionBase, maskAobPositionBase);
	AddyMsg(dwAddyPositionBase);
	if (dwAddyPositionBase == 0x0)
		return false;
	dwReturnPositionBase = 0x9F + dwAddyPositionBase;

	return true;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

//~~~~~~~~~~~ asm definition ~~~~~~~~~~~//
void __declspec(naked) newmemTexturesAndMasksPos(){
	__asm {
		//player texture
		push ecx 
		mov ecx, dword ptr[dwTexturePositionBase]
		mov ecx, dword ptr[edx + ecx] // 0x44 steps for npcs textures
		mov dword ptr [dwPlayerTexturePos], ecx
		mov word ptr[wPlayerTextureX], cx
		shr ecx, 0x10
		mov word ptr[wPlayerTextureY], cx
		pop ecx 

		//player mask
		push ecx
		mov ecx, dword ptr[dwMaskPositionBase]
		mov ecx, dword ptr[edx + ecx] // 0x24 steps for npcs masks
		mov dword ptr[dwPlayerMaskPos], ecx
		mov word ptr[wPlayerMaskX], cx
		shr ecx, 0x10
		mov word ptr[wPlayerMaskY], cx
		pop ecx

		//count of rendered entities
		push ecx
		mov byte ptr [bNPCCount], 0x00
		mov ecx, dword ptr[dwPlayerTexturePos]
		mov ecx, [dwNotUsedPositionMemory]

	countEntityLoop:
		push eax
		push edx
		inc[bNPCCount]
		mov eax, dword ptr[bNPCCount]
		mov edx, 0x44
		mul edx
		pop edx
		mov eax, dword ptr[edx + 0x20650 + eax]
		cmp ecx, eax
		pop eax
		jne countEntityLoop
		dec[bNPCCount]
		
		pop ecx

		// get invalid player texture positions
		push ecx
		push eax

		xor ecx, ecx
		mov[lOffset], cx
		mov cx, [bNPCCount]
		//dec cx
		inc ax
		inc ax
		cmp cx, ax
		jl outOfThisRoutine

	nextSubtaskLoop:
		mov eax, ecx
		push edx
		mov edx, 0x44
		mul edx
		pop edx
		mov eax, dword ptr[edx + 0x20650 + eax]
		cmp eax, dword ptr [edx + 0x20650]
		jne skip
		mov [lOffset], cx
	skip:
		xor eax, eax
		inc ax
		cmp cx, ax

		je outOfThisRoutine
		dec cx
		jmp nextSubtaskLoop
	outOfThisRoutine:
		pop eax
		pop ecx

		//DEBUG
		//mov[bNPCCount], 2
		cmp[bNPCCount], 0x0
		je sk
		dec[bNPCCount]
	sk:
		cmp[bNPCCount], 0x2
		jl skipAllRoutines
		//

		push ecx
		xor ecx, ecx
		//inc cx
		//inc cx
		//cmp cx, [bNPCCount]
		//ja skipAllRoutines

		mov cx, [bNPCCount]

		push eax
	setNewNPCPositionLoop:
		mov eax, ecx
		push edx
		mov edx, 0x44
		mul edx
		pop edx

		cmp cx, 0x0000
		jna skipNewPos

		push ebx
		//mov ebx, [arrPlayerPos+4*ecx]
		mov ebx, dword ptr[edx + 0x20650 + eax]
		cmp ebx, [dwPlayerTexturePos]
		je skipNewPos
		mov ebx, [arrPlayerPos + 4 * ecx]
		mov dword ptr[edx + 0x20650 + eax], ebx
	skipNewPos:
		pop ebx
		dec cx
		cmp cx, 0x0000
		ja setNewNPCPositionLoop

		//debug

		pop eax
	skipAllRoutines:
		pop ecx

		//original function
		movzx eax, word ptr[eax + edx]
		jmp dwReturnPositionBase //adapted jump
	}
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

//~~~~~~~~~~~ write asm to aob memory ~~~~~~~~~~~//
bool writeNewmem() {
	if (!Jump(dwAddyPositionBase, newmemTexturesAndMasksPos, nopcountPositionBase)) return false;
	return true;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

//~~~~~~~~~~~ restore aob memory ~~~~~~~~~~~//
void restoreMem() {
	writeBytes(dwAddyPositionBase, aobPositionBase, 9);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

void hud(){
	//system("CLS");
	clear();
	std::cout << "Memory view:" << std::endl;
	std::cout << std::endl;
	std::cout << "Player texture position: " << std::hex << dwPlayerTexturePos << std::endl;
	std::cout << "X: " << std::hex << wPlayerTextureX << std::endl;
	std::cout << "Y: " << std::hex << wPlayerTextureY << std::endl;
	std::cout << std::endl;
	std::cout << "Player mask position: " << std::hex << dwPlayerMaskPos << std::endl;
	std::cout << "X: " << std::hex << wPlayerMaskX << std::endl;
	std::cout << "Y: " << std::hex << wPlayerMaskY << std::endl;
	std::cout << std::endl;
	std::cout << "Count of rendered entities: " << std::hex << bNPCCount << std::endl;
	std::cout << std::endl;
	std::cout << "Offset index: " << std::hex << lOffset << std::endl;

	std::cout << std::endl;
	for (int i = 0; i < 15; i++) {
		std::cout << std::hex << arrPlayerPos[i] << std::endl;
	}
}

struct ExtraThread {
	virtual int func() {
		AllocConsole();
		freopen("CONOUT$", "w", stdout);
		std::cout << "hacking." << std::endl;

		if (!calcAobs()) {
			std::cout << "error: memory does not match my definitions" << std::endl;
			getchar();
			exit(true);
		}

		//system("CLS");
		std::cout << "hacking.." << std::endl;
			
		if (!writeNewmem()) {
			std::cout << "error: memory writing failed" << std::endl;
			getchar();
			exit(true);
		}

		//system("CLS");
		std::cout << "hacking..." << std::endl;
		while (1) {
			checkNewPlayerPosition();
			hud();
			Sleep(100);

			if (GetAsyncKeyState(VK_SPACE)) {
				restoreMem();
				std::cout << "info: exiting... memory may has been restored" << std::endl;
				break;
			}
		}
		return 0;
	}

	static DWORD WINAPI ThreadFunc(void* pContext) {
		ExtraThread *et = static_cast<ExtraThread*>(pContext);
		return et->func();
	}

};

#endif
