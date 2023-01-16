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

static
BOOL BangDebugLogA(struct CreateProcessPacketA *CreateProcessPacket);
static
BOOL BangDebugLogW(struct CreateProcessPacketW *CreateProcessPacket);

static DWORD BangDebugFlags = 0;

BOOL BangPreprocessPacketA(struct CreateProcessPacketA *CreateProcessPacket)
{
    if (BangDebugFlags)
        return BangDebugLogA(CreateProcessPacket);

    return TRUE;
}

BOOL BangPreprocessPacketW(struct CreateProcessPacketW *CreateProcessPacket)
{
    if (BangDebugFlags)
        return BangDebugLogW(CreateProcessPacket);

    return TRUE;
}

__declspec(dllexport)
VOID BangSetDebugFlags(DWORD DebugFlags)
{
    BangDebugFlags = DebugFlags;
}

static
BOOL BangDebugLogA(struct CreateProcessPacketA *CreateProcessPacket)
{
    CHAR Buf[1024];
    DWORD BytesWritten;

    wsprintfA(Buf, "bang: %s\n", CreateProcessPacket->lpApplicationName);
    WriteFile(GetStdHandle(STD_ERROR_HANDLE), Buf, lstrlenA(Buf), &BytesWritten, 0);

    return TRUE;
}

static
BOOL BangDebugLogW(struct CreateProcessPacketW *CreateProcessPacket)
{
    CHAR Buf[1024];
    DWORD BytesWritten;

    wsprintfA(Buf, "bang: %S", CreateProcessPacket->lpApplicationName);
    WriteFile(GetStdHandle(STD_ERROR_HANDLE), Buf, lstrlenA(Buf), &BytesWritten, 0);

    return TRUE;
}
