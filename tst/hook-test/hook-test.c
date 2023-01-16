/*
 * hook-test.c
 *
 * Copyright 2023 Bill Zissimopoulos
 */
/*
 * This file is part of Bang.
 *
 * It is licensed under the MIT license. The full license text can be found
 * in the License.txt file at the root of this project.
 */

#include <windows.h>
#include <stdio.h>

__declspec(dllimport)
VOID BangSetDebugFlags(DWORD DebugFlags);

int main(int argc, char **argv)
{
    CHAR ModuleFileName[MAX_PATH], *ModuleFileNameSuffix;
    CHAR CommandLine[1024];
    STARTUPINFOA StartupInfo;
    PROCESS_INFORMATION ProcessInformation;
    BOOL Success;
    unsigned long Count = 0;

    if (2 <= argc)
        Count = strtoul(argv[1], 0, 10);

    printf("%lu\n", Count);
    if (0 == Count--)
        return 0;

    BangSetDebugFlags(1);

    GetModuleFileNameA(0, ModuleFileName, MAX_PATH);
    ModuleFileNameSuffix = ModuleFileName + strlen(ModuleFileName) - (sizeof "64.exe" - 1);
    if (0 == strcmp(ModuleFileNameSuffix, "64.exe"))
        ModuleFileNameSuffix[0] = '3', ModuleFileNameSuffix[1] = '2';
    else
    if (0 == strcmp(ModuleFileNameSuffix, "32.exe"))
        ModuleFileNameSuffix[0] = '6', ModuleFileNameSuffix[1] = '4';
    snprintf(CommandLine, sizeof CommandLine, "\"%s\" %lu", ModuleFileName, Count);

    memset(&StartupInfo, 0, sizeof StartupInfo);
    Success = CreateProcessA(
        ModuleFileName,
        CommandLine,
        0,
        0,
        FALSE,
        0,
        0,
        0,
        &StartupInfo,
        &ProcessInformation);

    return Success ? 0 : GetLastError();
}
