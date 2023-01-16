/*
 * dll/library.h
 *
 * Copyright 2023 Bill Zissimopoulos
 */
/*
 * This file is part of Bang.
 *
 * It is licensed under the MIT license. The full license text can be found
 * in the License.txt file at the root of this project.
 */

#ifndef BANG_DLL_LIBRARY_H_INCLUDED
#define BANG_DLL_LIBRARY_H_INCLUDED

#include <windows.h>
#include <detours.h>

extern CHAR ModuleFileNameA[];

struct CreateProcessPacketA
{
    /* result */
    PROCESS_INFORMATION ProcessInformation;
    /* arguments */
    HANDLE hToken;
    LPCSTR lpApplicationName;
    LPSTR lpCommandLine;
    LPSECURITY_ATTRIBUTES lpProcessAttributes;
    LPSECURITY_ATTRIBUTES lpThreadAttributes;
    BOOL bInheritHandles;
    DWORD dwCreationFlags;
    LPVOID lpEnvironment;
    LPCSTR lpCurrentDirectory;
    LPSTARTUPINFOA lpStartupInfo;
    LPPROCESS_INFORMATION lpProcessInformation;
    /* buffers */
    CHAR ApplicationName[MAX_PATH];
    CHAR CommandLine[32767];
};

struct CreateProcessPacketW
{
    /* result */
    PROCESS_INFORMATION ProcessInformation;
    /* arguments */
    HANDLE hToken;
    LPCWSTR lpApplicationName;
    LPWSTR lpCommandLine;
    LPSECURITY_ATTRIBUTES lpProcessAttributes;
    LPSECURITY_ATTRIBUTES lpThreadAttributes;
    BOOL bInheritHandles;
    DWORD dwCreationFlags;
    LPVOID lpEnvironment;
    LPCWSTR lpCurrentDirectory;
    LPSTARTUPINFOW lpStartupInfo;
    LPPROCESS_INFORMATION lpProcessInformation;
    LPCWSTR lpUsername;
    LPCWSTR lpDomain;
    LPCWSTR lpPassword;
    DWORD dwLogonFlags;
    /* buffers */
    WCHAR ApplicationName[MAX_PATH];
    WCHAR CommandLine[32767];
};

VOID HookCreateProcess(BOOL Flag,
    BOOL (*PreprocessA)(struct CreateProcessPacketA *),
    BOOL (*PreprocessW)(struct CreateProcessPacketW *));

BOOL BangPreprocessPacketA(struct CreateProcessPacketA *CreateProcessPacket);
BOOL BangPreprocessPacketW(struct CreateProcessPacketW *CreateProcessPacket);

#endif
