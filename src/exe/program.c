/*
 * exe/program.c
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
#include <winternl.h>
#include <psapi.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>

static void warn(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);
}

static void usage()
{
    fprintf(stderr, ""
        "usage:\n"
        "  bang [-p -1|PID]\n"
        "  bang COMMAND [ARG ...]\n");

    exit(2);
}

static int attach(int argc, char **argv)
{
    CHAR ModuleFileName[MAX_PATH + sizeof "bang64.dll" - 1], *ModuleFileNameSuffix;
    DWORD ProcessId;
    HANDLE Process = 0;
    DWORD RemoteModule32Count = 0;
    PVOID RemoteBuffer = 0;
    HANDLE RemoteThread = 0;
    DWORD RemoteExitCode;
    SIZE_T BytesWritten;
    int ExitCode;

    if (2 > argc)
        usage();

    ProcessId = strtoul(argv[1], 0, 10);

    if (-1 == ProcessId)
    {
        PROCESS_BASIC_INFORMATION BasicInfo = { 0 };
        ULONG Length;
        NTSTATUS Status = NtQueryInformationProcess(
            GetCurrentProcess(),
            ProcessBasicInformation,
            &BasicInfo,
            sizeof BasicInfo,
            &Length);
        if (!NT_SUCCESS(Status))
        {
            warn("cannot find parent process id");
            ExitCode = 1;
            goto exit;
        }
        ProcessId = (DWORD)(ULONG_PTR)BasicInfo.Reserved3;
    }

    Process = OpenProcess(
        PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE,
        FALSE,
        ProcessId);
    if (0 == Process)
    {
        warn("cannot open process id %lu", ProcessId);
        ExitCode = 1;
        goto exit;
    }

    GetModuleFileNameA(0, ModuleFileName, MAX_PATH);
    ModuleFileNameSuffix = strrchr(ModuleFileName, '\\');
    ModuleFileNameSuffix = ModuleFileNameSuffix ? ModuleFileNameSuffix + 1 : ModuleFileName;

    EnumProcessModulesEx(Process, 0, 0, &RemoteModule32Count, LIST_MODULES_32BIT);
    if (0 == RemoteModule32Count)
        strcpy(ModuleFileNameSuffix, "bang64.dll");
    else
        strcpy(ModuleFileNameSuffix, "bang32.dll");

    RemoteBuffer = VirtualAllocEx(Process, 0, 4096, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (0 == RemoteBuffer)
    {
        warn("cannot allocate memory in process id %lu", ProcessId);
        ExitCode = 1;
        goto exit;
    }

    if (!WriteProcessMemory(Process, RemoteBuffer, ModuleFileName, strlen(ModuleFileName) + 1, &BytesWritten))
    {
        warn("cannot write memory in process id %lu", ProcessId);
        ExitCode = 1;
        goto exit;
    }

    RemoteThread = CreateRemoteThread(Process, 0, 0, (LPTHREAD_START_ROUTINE)LoadLibraryA, RemoteBuffer, 0, 0);
    if (0 == RemoteThread)
    {
        warn("cannot create thread in process id %lu", ProcessId);
        ExitCode = 1;
        goto exit;
    }

    WaitForSingleObject(RemoteThread, INFINITE);
    GetExitCodeThread(RemoteThread, &RemoteExitCode);
    if (0 == RemoteExitCode)
    {
        warn("cannot load %s in process id %lu", ModuleFileNameSuffix, ProcessId);
        ExitCode = 1;
        goto exit;
    }

    ExitCode = 0;

exit:
    if (0 != RemoteThread)
        CloseHandle(RemoteThread);

    if (0 != RemoteBuffer)
        VirtualFreeEx(Process, RemoteBuffer, 0, MEM_RELEASE);

    if (0 != Process)
        CloseHandle(Process);

    return ExitCode;
}

static int spawn(int argc, char **argv)
{
    return 0 == _spawnvp(_P_NOWAIT, argv[0], argv);
}

int main(int argc, char **argv)
{
    if (2 > argc)
        usage();

    if (0 == strcmp("-p", argv[1]))
        return attach(argc - 1, argv + 1);
    else
        return spawn(argc - 1, argv + 1);
}
