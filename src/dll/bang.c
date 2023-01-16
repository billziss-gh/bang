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

BOOL BangPreprocessPacketA(struct CreateProcessPacketA *CreateProcessPacket)
{
    return TRUE;
}

BOOL BangPreprocessPacketW(struct CreateProcessPacketW *CreateProcessPacket)
{
    return TRUE;
}
