/*
 * dll/bang.c
 *
 * Copyright 2023 Bill Zissimopoulos
 */
/*
 * This file is part of Bang.
 *
 * It is licensed under the MIT license. The full license text can be found
 * in the License.txt file at the root of this project.
 */

#include <dll/library.h>

 
/*
 * force load
 */

__declspec(dllexport)
VOID BangLoad(VOID)
{
}


/*
 * debugging
 */

static DWORD BangDebugFlags = 0;

static
VOID BangDebugLogA(struct CreateProcessPacketA *CreateProcessPacket)
{
    CHAR Buf[1024];
    DWORD Bytes;
    wsprintfA(Buf, "bang: %s\n", CreateProcessPacket->lpApplicationName);
    WriteFile(GetStdHandle(STD_ERROR_HANDLE), Buf, lstrlenA(Buf), &Bytes, 0);
}

static
VOID BangDebugLogW(struct CreateProcessPacketW *CreateProcessPacket)
{
    CHAR Buf[1024];
    DWORD Bytes;
    wsprintfA(Buf, "bang: %S", CreateProcessPacket->lpApplicationName);
    WriteFile(GetStdHandle(STD_ERROR_HANDLE), Buf, lstrlenA(Buf), &Bytes, 0);
}

__declspec(dllexport)
VOID BangSetDebugFlags(DWORD DebugFlags)
{
    BangDebugFlags = DebugFlags;
}


/*
 * argument handling
 */

static
BOOL BangParseCommandLineA(PSTR CommandLine, int *PArgc, PSTR **PArgv)
{
    PWSTR CommandLineW = 0, *ArgvW = 0;
    PSTR *Argv = 0, Text;
    int Length, Total, Copied;

    *PArgc = 0;
    *PArgv = 0;

    Length = lstrlenA(CommandLine);
    CommandLineW = HeapAlloc(GetProcessHeap(), 0, (Length + 1) * 4);
    if (0 == CommandLineW)
        goto exit;

    if (0 == MultiByteToWideChar(CP_UTF8, 0, CommandLine, Length + 1, CommandLineW, (Length + 1) * 4))
        goto exit;

    ArgvW = CommandLineToArgvW(CommandLineW, PArgc);
    if (0 == ArgvW)
        goto exit;

    Total = 0;
    for (int I = 0; *PArgc > I; I++)
    {
        if (0 == (Length = WideCharToMultiByte(CP_UTF8, 0, ArgvW[I], -1, 0, 0, 0, 0)))
            goto exit;
        Total += Length;
    }

    Argv = LocalAlloc(LMEM_ZEROINIT, (*PArgc + 1) * sizeof(PVOID) + Total);
    if (0 == Argv)
        goto exit;
    Text = (PUINT8)Argv + (*PArgc + 1) * sizeof(PVOID);

    Copied = 0;
    for (int I = 0; *PArgc > I; I++)
    {
        Argv[I] = Text + Copied;
        if (0 == (Length = WideCharToMultiByte(CP_UTF8, 0, ArgvW[I], -1, Argv[I], Total - Copied, 0, 0)))
            goto exit;
        Copied += Length;
    }
    Argv[*PArgc] = 0;

    *PArgv = Argv;
    Argv = 0;

exit:
    if (0 != Argv)
        LocalFree(Argv);

    if (0 != ArgvW)
        LocalFree(ArgvW);

    if (0 != CommandLineW)
        HeapFree(GetProcessHeap(), 0, CommandLineW);

    return 0 != *PArgv;
}

static
BOOL BangParseCommandLineW(PWSTR CommandLine, int *PArgc, PWSTR **PArgv)
{
    *PArgc = 0;
    *PArgv = CommandLineToArgvW(CommandLine, PArgc);
    return 0 != *PArgv;
}

static
int BangMultiByteToXCharA(
    PSTR SourceString, int SourceLength,
    PSTR DestinationString, int DestinationLength)
{
    if (SourceLength > DestinationLength)
        return 0;
    memcpy(DestinationString, SourceString, SourceLength);
    return SourceLength;
}

int BangMultiByteToXCharW(
    PSTR SourceString, int SourceLength,
    PWSTR DestinationString, int DestinationLength)
{
    return MultiByteToWideChar(CP_UTF8, 0,
        SourceString, SourceLength,
        DestinationString, DestinationLength);
}


/*
 * bang execution
 */

#undef  XTYP
#define XTYP                            CHAR
#undef  XLIT
#define XLIT(S)                         S
#undef  XSYM
#define XSYM(N)                         N ## A
#include "bang.i"

#undef  XTYP
#define XTYP                            WCHAR
#undef  XLIT
#define XLIT(S)                         L ## S
#undef  XSYM
#define XSYM(N)                         N ## W
#include "bang.i"
