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

VOID BangPreprocessPacketA(struct CreateProcessPacketA *CreateProcessPacket)
{
    if (BangDebugFlags)
    {
        BangDebugLogA(CreateProcessPacket);
        return;
    }
}

VOID BangPreprocessPacketW(struct CreateProcessPacketW *CreateProcessPacket)
{
    if (BangDebugFlags)
    {
        BangDebugLogW(CreateProcessPacket);
        return;
    }
}
