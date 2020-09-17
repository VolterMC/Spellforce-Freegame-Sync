#include <Windows.h>
#include <ctime>
#include <string>
#include <iostream>

#define SYNC_RATE 60.0
#define MSG_RATE  1

const DWORD gBasePtr = 0x00D6A5C0;
const DWORD resourceOffsets[3][14] = {
  {0x19B, 0x19F, 0x1A7, 0x1AB, 0x1B3, 0x1DF, 0x1E3, 0x1EB, 0x1EF, 0x1F7, 0x1FB, 0x203, 0x22F, 0x233},
  {0x327, 0x32B, 0x333, 0x337, 0x33F, 0x36B, 0x36F, 0x377, 0x37B, 0x383, 0x387, 0x38F, 0x3BB, 0x3BF},
  {0x4B3, 0x4B7, 0x4BF, 0x4C3, 0x4CB, 0x4F7, 0x4FB, 0x503, 0x507, 0x50F, 0x513, 0x51B, 0x547, 0x54B}
};

int main() {
  HWND        gameWindowHandle = NULL;
  HANDLE      procHandle = NULL;
  DWORD       procId = 0;
  BOOLEAN     isGamePresent = FALSE;
  std::string message = "";
  clock_t     messageTick = clock();
  clock_t     syncTick = clock();
  DWORD       baseAddr = 0;
  DWORD       currentResources[3][14] = { 0 };
  DWORD       previousResources[3][14] = { 0 };
  INT         relativeDeltas[3][14] = { 0 };

  while (!GetAsyncKeyState(VK_PAUSE)) {
    if ((clock() - messageTick) > (1000 / MSG_RATE)) {
      messageTick = clock();
      isGamePresent = FALSE;

      gameWindowHandle = FindWindow(NULL, L"SpellForce");
      if (gameWindowHandle != NULL) {
        GetWindowThreadProcessId(gameWindowHandle, &procId);
        if (procId != 0) {
          procHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, procId);
          if (procHandle != NULL) {
            isGamePresent = TRUE;
            ReadProcessMemory(procHandle, (LPCVOID)gBasePtr, &baseAddr, sizeof(DWORD), NULL);
            message = "SpellForce FreeGame synchronizer working!\n";
          }
        }
        else {
          message = "Cannot obtain SpellForce ProcessID!\n";
        }
      }
      else {
        message = "SpellForce window not found!\n";
      }

      system("cls");
      std::cout << "----SpellForce FreeGame synchronizer by VolterMC----\n";
      std::cout << "----          Press PAUSE key to exit!          ----\n";
      std::cout << "STATUS: " << message;
    }

    if ((clock() - syncTick) > (1000 / SYNC_RATE) && isGamePresent) {
      // Magic!
      // Read
      for (UINT8 player = 0; player < 3; ++player) {
        for (UINT8 resource = 0; resource < 14; ++resource) {
          ReadProcessMemory(procHandle, (LPCVOID)(baseAddr + resourceOffsets[player][resource]), &currentResources[player][resource], sizeof(DWORD), NULL);
        }
      }

      // Compare
      INT absoluteDeltas[3][14] = { 0 };
      for (UINT8 resource = 0; resource < 14; ++resource) {
        for (UINT8 player = 0; player < 3; ++player) {
          absoluteDeltas[player][resource] = currentResources[player][resource] - previousResources[player][resource] - relativeDeltas[player][resource];
        }
        relativeDeltas[0][resource] = absoluteDeltas[1][resource] + absoluteDeltas[2][resource];
        relativeDeltas[1][resource] = absoluteDeltas[0][resource] + absoluteDeltas[2][resource];
        relativeDeltas[2][resource] = absoluteDeltas[0][resource] + absoluteDeltas[1][resource];
      }

      // Correct, copy and commit
      for (UINT8 player = 0; player < 3; ++player) {
        for (UINT8 resource = 0; resource < 14; ++resource) {
          previousResources[player][resource] = currentResources[player][resource] + relativeDeltas[player][resource];
          WriteProcessMemory(procHandle, (LPVOID)(baseAddr + resourceOffsets[player][resource]), &previousResources[player][resource], sizeof(DWORD), NULL);
        }
      }
    }
  }

  if (procHandle != NULL) {
    CloseHandle(procHandle);
  }

  if (gameWindowHandle != NULL) {
    CloseHandle(gameWindowHandle);
  }
  return 0;
}