/*
 * dll/bang.i
 *
 * Copyright 2023 Bill Zissimopoulos
 */
/*
 * This file is part of Bang.
 *
 * It is licensed under the MIT license. The full license text can be found
 * in the License.txt file at the root of this project.
 */


/*
 * argument handling
 */

static
BOOL XSYM(BangReplaceArguments)(XTYP *String, int Argc, XTYP **Argv, XTYP **PNewString)
{
    XTYP *NewString = 0, *P, *Q;
    int Length, L;

    *PNewString = 0;

    Length = 0;
    for (P = String; *P; P++)
        switch (*P)
        {
        case '$':
            if ('\0' == P[1])
                goto default_length;
            P++;
            if ('0' <= *P && *P <= '9' && Argc > *P - '0')
                Length += XSYM(lstrlen)(Argv[*P - '0']);
            break;
#if 0
        /* backslash is the path sep on Windows, so using it for escape is asking for trouble */
        case '\\':
            if ('\0' == P[1])
                goto default_length;
            P++;
            /* fallthrough */
#endif
        default:
        default_length:
            Length++;
            break;
        }

    NewString = HeapAlloc(GetProcessHeap(), 0, (Length + 1) * sizeof(XTYP));
    if (0 == NewString)
        return FALSE;

    for (P = String, Q = NewString; *P; P++)
        switch (*P)
        {
        case '$':
            if ('\0' == P[1])
                goto default_copy;
            P++;
            if ('0' <= *P && *P <= '9' && Argc > *P - '0')
            {
                L = XSYM(lstrlen)(Argv[*P - '0']);
                memcpy(Q, Argv[*P - '0'], L * sizeof(XTYP));
                Q += L;
            }
            break;
#if 0
        /* backslash is the path sep on Windows, so using it for escape is asking for trouble */
        case '\\':
            if ('\0' == P[1])
                goto default_copy;
            P++;
            /* fallthrough */
#endif
        default:
        default_copy:
            *Q++ = *P;
            break;
        }
    *Q = '\0';

    *PNewString = NewString;

    return TRUE;
}


/*
 * bang execution
 */

struct XSYM(BangInterpreterMapEntry)
{
    XTYP *PosixPath, *WindowsPath;
};
static struct XSYM(BangInterpreterMapEntry) XSYM(BangInterpreterMap)[] =
{
    { XLIT("/usr/bin/env"), 0 },
    { XLIT("/usr/bin/"), XLIT("%SYSTEMROOT%\\System32\\") },
    { XLIT("/bin/"), XLIT("%SYSTEMROOT%\\System32\\") },
};

static
BOOL XSYM(BangRemapInterpreter)(struct XSYM(CreateProcessPacket) *CreateProcessPacket,
    CHAR **PRestOfLine)
{
    XTYP *PosixPath, *WindowsPath, ApplicationName[MAX_PATH];
    int PosixLength, WindowsLength, Length;

    for (int I = 0; sizeof XSYM(BangInterpreterMap) / sizeof XSYM(BangInterpreterMap)[0] > I; I++)
    {
        PosixPath = XSYM(BangInterpreterMap)[I].PosixPath;
        PosixLength = XSYM(lstrlen)(PosixPath);
        if (0 < PosixLength && '/' == PosixPath[PosixLength - 1])
        {
            if (0 == XSTR(ncmp)(CreateProcessPacket->ApplicationName, PosixPath, PosixLength))
            {
                WindowsPath = XSYM(BangInterpreterMap)[I].WindowsPath;
                WindowsLength = XSYM(lstrlen)(WindowsPath);
                Length = XSYM(lstrlen)(CreateProcessPacket->ApplicationName + PosixLength);

                if (WindowsLength + Length + sizeof ".exe" > MAX_PATH)
                    return FALSE;

                memcpy(
                    ApplicationName,
                    WindowsPath,
                    WindowsLength * sizeof(XTYP));
                memcpy(
                    ApplicationName + WindowsLength,
                    CreateProcessPacket->ApplicationName + PosixLength,
                    Length * sizeof(XTYP));
                memcpy(
                    ApplicationName + WindowsLength + Length,
                    XLIT(".exe"),
                    sizeof ".exe" * sizeof(XTYP));

                if (0 == XSYM(ExpandEnvironmentStrings)(
                    ApplicationName, CreateProcessPacket->ApplicationName, MAX_PATH))
                    return FALSE;

                break;
            }
        }
        else
        if (0 == XSTR(cmp)(CreateProcessPacket->ApplicationName, PosixPath))
        {
            WindowsPath = XSYM(BangInterpreterMap)[I].WindowsPath;
            if (0 == WindowsPath)
            {
                CHAR *Interpreter, *P;

                for (P = *PRestOfLine; *P && (' ' == *P || '\t' == *P); P++)
                    ;
                Interpreter = P;

                for (; *P && ' ' != *P && '\t' != *P; P++)
                    ;
                *PRestOfLine = P;

                if (0 == (Length = XSYM(BangMultiByteToXChar)(
                    Interpreter,
                    (int)(*PRestOfLine - Interpreter),
                    ApplicationName,
                    MAX_PATH - 1)))
                    return FALSE;
                ApplicationName[Length] = '\0';

                Length = XSYM(SearchPath)(
                    0, ApplicationName, XLIT(".exe"), MAX_PATH, CreateProcessPacket->ApplicationName, 0);
                if (0 == Length || MAX_PATH <= Length)
                    return FALSE;

                break;
            }
            else
            {
                WindowsLength = XSYM(lstrlen)(WindowsPath);

                if (WindowsLength >= MAX_PATH)
                    return FALSE;

                memcpy(
                    ApplicationName,
                    WindowsPath,
                    (WindowsLength + 1) * sizeof(XTYP));

                if (0 == XSYM(ExpandEnvironmentStrings)(
                    ApplicationName, CreateProcessPacket->ApplicationName, MAX_PATH))
                    return FALSE;

                break;
            }
        }
    }

    return TRUE;
}

static
BOOL XSYM(BangExecuteInterpreter)(struct XSYM(CreateProcessPacket) *CreateProcessPacket,
    CHAR *Line)
{
    CHAR *Interpreter, *RestOfLine, *P;
    XTYP **Argv = 0, *NewRestOfLine = 0;
    int CommandLineLength, Length, Argc;
    BOOL Result = FALSE;

    Interpreter = Line + 2;
    for (P = Interpreter; *P && ' ' != *P && '\t' != *P; P++)
        ;
    RestOfLine = P;

    if (0 == (Length = XSYM(BangMultiByteToXChar)(
        Interpreter,
        (int)(RestOfLine - Interpreter),
        CreateProcessPacket->ApplicationName,
        MAX_PATH - 1)))
        goto exit;
    CreateProcessPacket->ApplicationName[Length] = '\0';

    if (!XSYM(BangRemapInterpreter)(CreateProcessPacket, &RestOfLine))
        goto exit;

    CommandLineLength = 0;
    Length = XSYM(lstrlen)(CreateProcessPacket->ApplicationName);
    if (2/* quotes */ + Length > MAX_COMMANDLINE - 1 - CommandLineLength)
        goto exit;
    CreateProcessPacket->CommandLine[CommandLineLength++] = '\"';
    memcpy(CreateProcessPacket->CommandLine + CommandLineLength,
        CreateProcessPacket->ApplicationName, Length * sizeof(XTYP));
    CommandLineLength += Length;
    CreateProcessPacket->CommandLine[CommandLineLength++] = '\"';
    CreateProcessPacket->CommandLine[CommandLineLength] = '\0';

    if ('#' == Line[0])
    {
        if (*RestOfLine)
        {
            if (0 == (Length = XSYM(BangMultiByteToXChar)(
                RestOfLine,
                lstrlenA(RestOfLine),
                CreateProcessPacket->CommandLine + CommandLineLength,
                MAX_COMMANDLINE - 1 - CommandLineLength)))
                goto exit;
            CommandLineLength += Length;
            CreateProcessPacket->CommandLine[CommandLineLength] = '\0';
        }

        if (0 != CreateProcessPacket->lpCommandLine)
        {
            Length = XSYM(lstrlen)(CreateProcessPacket->lpCommandLine);
            if (1/* separator space */ + Length > MAX_COMMANDLINE - 1 - CommandLineLength)
                goto exit;
            CreateProcessPacket->CommandLine[CommandLineLength++] = ' ';
            memcpy(CreateProcessPacket->CommandLine + CommandLineLength,
                CreateProcessPacket->lpCommandLine, Length * sizeof(XTYP));
            CommandLineLength += Length;
            CreateProcessPacket->CommandLine[CommandLineLength] = '\0';
        }
    }
    else
    if ('/' == Line[0])
    {
        if (*RestOfLine)
        {
            if (0 == (Length = XSYM(BangMultiByteToXChar)(
                RestOfLine,
                lstrlenA(RestOfLine),
                CreateProcessPacket->CommandLine + CommandLineLength,
                MAX_COMMANDLINE - 1 - CommandLineLength)))
                goto exit;
            CreateProcessPacket->CommandLine[CommandLineLength + Length] = '\0';

            if (!XSYM(BangParseCommandLine)(CreateProcessPacket->lpCommandLine, &Argc, &Argv))
                goto exit;
            if (!XSYM(BangReplaceArguments)(CreateProcessPacket->CommandLine + CommandLineLength,
                Argc, Argv, &NewRestOfLine))
                goto exit;

            Length = XSYM(lstrlen)(NewRestOfLine);
            if (Length > MAX_COMMANDLINE - 1 - CommandLineLength)
                goto exit;
            memcpy(CreateProcessPacket->CommandLine + CommandLineLength,
                NewRestOfLine, Length * sizeof(XTYP));
            CommandLineLength += Length;
            CreateProcessPacket->CommandLine[CommandLineLength] = '\0';
        }
    }

    CreateProcessPacket->lpApplicationName = CreateProcessPacket->ApplicationName;
    CreateProcessPacket->lpCommandLine = CreateProcessPacket->CommandLine;

    Result = TRUE;

exit:
    if (0 != NewRestOfLine)
        HeapFree(GetProcessHeap(), 0, NewRestOfLine);

    if (0 != Argv)
        LocalFree(Argv);

    return Result;
}

static
VOID XSYM(BangExecute)(struct XSYM(CreateProcessPacket) *CreateProcessPacket,
    XTYP *FilePath)
{
    HANDLE Handle = INVALID_HANDLE_VALUE;
    CHAR Buffer[4096];
    DWORD Bytes;

    Handle = XSYM(CreateFile)(
        FilePath,
        FILE_READ_DATA | FILE_EXECUTE,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        0,
        OPEN_EXISTING,
        0,
        0);
    if (INVALID_HANDLE_VALUE == Handle)
        goto exit;

    if (!ReadFile(Handle, Buffer, sizeof Buffer - 1, &Bytes, 0))
        goto exit;

    if (3 <= Bytes &&
        ('#' == Buffer[0] && '!' == Buffer[1] && '/' == Buffer[2]) ||
        ('/' == Buffer[0] && '/' == Buffer[1] && '/' == Buffer[2]))
    {
        for (CHAR *P = Buffer + 3, *EndP = Buffer + Bytes; EndP > P; P++)
            if ('\r' == *P || '\n' == *P)
            {
                *P = '\0';
                break;
            }
        Buffer[Bytes] = '\0';
        XSYM(BangExecuteInterpreter)(CreateProcessPacket, Buffer);
    }

exit:
    if (INVALID_HANDLE_VALUE != Handle)
        CloseHandle(Handle);
}

VOID XSYM(BangPreprocessPacket)(struct XSYM(CreateProcessPacket) *CreateProcessPacket)
{
    if (BangDebugFlags)
    {
        XSYM(BangDebugLog)(CreateProcessPacket);
        return;
    }

    if (0 == CreateProcessPacket->lpApplicationName &&
        0 == CreateProcessPacket->lpCommandLine)
        return;

    XTYP FilePathBuf[MAX_PATH], FileNameBuf[MAX_PATH];
    XTYP *StartP, *P;
    DWORD Length;

    FilePathBuf[0] = '\0';
    FileNameBuf[0] = '\0';

    if (0 != CreateProcessPacket->lpApplicationName)
    {
        Length = XSYM(lstrlen)(CreateProcessPacket->lpApplicationName);
        if (MAX_PATH > Length)
            memcpy(FilePathBuf, CreateProcessPacket->lpApplicationName, (Length + 1) * sizeof(XTYP));
    }
    else
    if (0 != CreateProcessPacket->lpCommandLine)
    {
        if ('\"' == CreateProcessPacket->lpCommandLine[0])
        {
            StartP = CreateProcessPacket->lpCommandLine + 1;
            for (P = StartP; *P && '\"' != *P; P++)
                ;
        }
        else
        {
            StartP = CreateProcessPacket->lpCommandLine;
            for (P = StartP; *P && ' ' != *P && '\t' != *P; P++)
                ;
        }

        if (P - StartP < MAX_PATH)
        {
            memcpy(FileNameBuf, StartP, (P - StartP) * sizeof(XTYP));
            FileNameBuf[P - StartP] = '\0';

            Length = XSYM(SearchPath)(0, FileNameBuf, 0, MAX_PATH, FilePathBuf, 0);
            if (0 == Length || MAX_PATH <= Length)
                FilePathBuf[0] = '\0';
        }
    }

    if (FilePathBuf[0])
        XSYM(BangExecute)(CreateProcessPacket, FilePathBuf);
}
