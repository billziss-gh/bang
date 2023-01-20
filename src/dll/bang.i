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
 * registry
 */

static inline
BOOL XSYM(BangRegGetValue)(XTYP *Name, XTYP *Buffer, PDWORD PLength)
{
    return ERROR_SUCCESS == XSYM(RegGetValue)(
        HKEY_CURRENT_USER,
        XLIT("Software\\Bang"),
        Name,
        RRF_RT_REG_SZ | RRF_RT_REG_EXPAND_SZ,
        0,
        Buffer,
        PLength);
}


/*
 * argument handling
 */

static
int XSYM(BangAllArgumentsLength)(int Argc, XTYP **Argv, BOOL Quote)
{
    int Length = 0, QuoteLength = Quote ? 2 : 0;
    for (int I = 1; Argc > I; I++)
        Length += (1 < I) + QuoteLength + XSYM(lstrlen)(Argv[I]);
    return Length;
}

static
XTYP *XSYM(BangAllArgumentsCopy)(int Argc, XTYP **Argv, BOOL Quote, XTYP *Q)
{
    int L = 0;
    for (int I = 1; Argc > I; I++)
    {
        if (1 < I)
            *Q++ = ' ';
        if (Quote)
            *Q++ = '\"';
        L = XSYM(lstrlen)(Argv[I]);
        memcpy(Q, Argv[I], L * sizeof(XTYP));
        Q += L;
        if (Quote)
            *Q++ = '\"';
    }
    return Q;
}

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
            else if ('@' == *P)
                Length += XSYM(BangAllArgumentsLength)(Argc, Argv, FALSE);
            break;
        case '\"':
            if ('$' == P[1] && '@' == P[2] && '\"' == P[3])
            {
                P += 3;
                Length += XSYM(BangAllArgumentsLength)(Argc, Argv, TRUE);
                break;
            }
            goto default_length;
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
            else if ('@' == *P)
                Q = XSYM(BangAllArgumentsCopy)(Argc, Argv, FALSE, Q);
            break;
        case '\"':
            if ('$' == P[1] && '@' == P[2] && '\"' == P[3])
            {
                P += 3;
                Q = XSYM(BangAllArgumentsCopy)(Argc, Argv, TRUE, Q);
                break;
            }
            goto default_length;
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

static
BOOL XSYM(BangRemapInterpreter)(struct XSYM(CreateProcessPacket) *CreateProcessPacket,
    CHAR **PRestOfLine)
{
    static XTYP DefaultPathmap[] = XLIT(
        "/usr/bin/env*;"
        "/usr/bin/*%SYSTEMROOT%\\System32\\;"
        "/bin/*%SYSTEMROOT%\\System32\\"
    );
    XTYP Pathmap[4096], *P, *Q;
    XTYP *PosixPath, *WindowsPath, ApplicationName[MAX_PATH];
    DWORD PosixLength, WindowsLength, Length;

    Length = sizeof Pathmap;
    if (!XSYM(BangRegGetValue)(XLIT("Pathmap"), Pathmap, &Length))
    {
        if (0 == XSYM(ExpandEnvironmentStrings)(
            DefaultPathmap, Pathmap, sizeof Pathmap / sizeof Pathmap[0]))
            return FALSE;
    }

    for (P = Pathmap; P && *P; P = Q)
    {
        Q = XSTR(chr)(P, '*');
        if (Q)
        {
            PosixPath = P;
            PosixLength = (DWORD)(Q - P);
            *Q++ = '\0';
        }
        else
            break;

        P = Q;
        Q = XSTR(chr)(P, ';');
        if (Q)
        {
            WindowsPath = P;
            WindowsLength = (DWORD)(Q - P);
            *Q++ = '\0';
        }
        else
        {
            WindowsPath = P;
            WindowsLength = XSYM(lstrlen)(P);
        }

        if (0 < PosixLength && '/' == PosixPath[PosixLength - 1] &&
            0 == XSTR(ncmp)(CreateProcessPacket->ApplicationName, PosixPath, PosixLength))
        {
            if ('\0' == *WindowsPath)
            {
                XSYM(lstrcpy)(ApplicationName, CreateProcessPacket->ApplicationName + PosixLength);

                Length = XSYM(SearchPath)(
                    0, ApplicationName, XLIT(".exe"), MAX_PATH, CreateProcessPacket->ApplicationName, 0);
                return 0 < Length && Length < MAX_PATH; /* term-0 not included in Length */
            }
            else
            {
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

                Length = XSYM(GetFullPathName)(
                    ApplicationName, MAX_PATH, CreateProcessPacket->ApplicationName, 0);
                return 0 < Length && Length <= MAX_PATH; /* term-0 included in Length */
            }
        }
        else
        if (0 == XSTR(cmp)(CreateProcessPacket->ApplicationName, PosixPath))
        {
            if ('\0' == *WindowsPath)
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
                return 0 < Length && Length < MAX_PATH; /* term-0 not included in Length */
            }
            else
            {
                if (WindowsLength >= MAX_PATH)
                    return FALSE;

                memcpy(
                    ApplicationName,
                    WindowsPath,
                    (WindowsLength + 1) * sizeof(XTYP));

                Length = XSYM(GetFullPathName)(
                    ApplicationName, MAX_PATH, CreateProcessPacket->ApplicationName, 0);
                return 0 < Length && Length <= MAX_PATH; /* term-0 included in Length */
            }
        }
    }

    return FALSE;
}

static
BOOL XSYM(BangExecuteInterpreter)(struct XSYM(CreateProcessPacket) *CreateProcessPacket,
    CHAR *Line)
{
    CHAR *Interpreter, *RestOfLine, *P;
    XTYP **Argv = 0, *NewRestOfLine = 0, *ExitCommand, *X;
    int CommandLineLength, Length, Argc;
    BOOL Result = FALSE;

    CreateProcessPacket->CommandLine = HeapAlloc(GetProcessHeap(), 0, MAX_COMMANDLINE);
    if (0 == CreateProcessPacket->CommandLine)
        goto exit;

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

            /* strip "; exit at the end */
            for (X = CreateProcessPacket->CommandLine + CommandLineLength + Length - 1;
                CreateProcessPacket->CommandLine + CommandLineLength <= X && ';' != *X && '\"' != *X;
                X--)
                ;
            if (CreateProcessPacket->CommandLine + CommandLineLength <= X && ';' == *X)
            {
                ExitCommand = X;
                for (X++; *X && (' ' == *X || '\t' == *X); X++)
                    ;
                if (0 == XSTR(ncmp)(X, XLIT("exit"), 4))
                {
                    for (X += 4; *X && (' ' == *X || '\t' == *X); X++)
                        ;
                    if ('\0' == *X)
                        *ExitCommand = '\0';
                }
            }

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
    const XTYP *FilePath, PBOOL PIsExecutable)
{
    HANDLE Handle = INVALID_HANDLE_VALUE;
    CHAR Buffer[4096];
    DWORD Bytes;

    if (0 != PIsExecutable)
        *PIsExecutable = FALSE;

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

        if (0 != PIsExecutable)
            *PIsExecutable = TRUE;
        else
            XSYM(BangExecuteInterpreter)(CreateProcessPacket, Buffer);
    }

exit:
    if (INVALID_HANDLE_VALUE != Handle)
        CloseHandle(Handle);
}

static
BOOL XSYM(BangShouldExecute)(const XTYP *FilePath)
{
    /* FilePath should be full path */

    XTYP Directories[4096], DirName[MAX_PATH], *P, *Q;
    DWORD Length;

    Length = sizeof Directories;
    if (!XSYM(BangRegGetValue)(XLIT("Directories"), Directories, &Length))
        return TRUE;

    XSYM(lstrcpy)(DirName, FilePath);
    P = XSTR(rchr)(DirName, '\\');
    if (0 != P)
        *++P = '\0';
    else
        return FALSE;

    for (P = Directories; P && *P; P = Q)
    {
        Q = XSTR(chr)(P, ';');
        if (Q)
            *Q++ = '\0';
        if (0 == XSTR(nicmp)(DirName, P, XSYM(lstrlen)(P)))
            return TRUE;
    }

    return FALSE;
}

static
BOOL XSYM(BangShouldDetour)(const XTYP *FilePath)
{
    /* FilePath should be full path */

    XTYP Programs[4096], *BaseName, *P, *Q;
    DWORD Length;

    Length = sizeof Programs;
    if (!XSYM(BangRegGetValue)(XLIT("Programs"), Programs, &Length))
        return TRUE;

    BaseName = XSTR(rchr)(FilePath, '\\');
    if (0 != BaseName)
        BaseName++;
    else
        return FALSE;

    for (P = Programs; P && *P; P = Q)
    {
        Q = XSTR(chr)(P, ';');
        if (Q)
            *Q++ = '\0';
        if (0 != XSTR(chr)(P, '\\'))
        {
            /* match full path; we do not care about alternate names from symlinks, etc. */
            if (0 == XSTR(icmp)(FilePath, P))
                return TRUE;
        }
        else
        {
            /* match base name */
            if (0 == XSTR(icmp)(BaseName, P))
                return TRUE;
        }
    }

    return FALSE;
}

VOID XSYM(BangBeforeCreateProcess)(struct XSYM(CreateProcessPacket) *CreateProcessPacket)
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
        Length = XSYM(GetFullPathName)(
            CreateProcessPacket->lpApplicationName, MAX_PATH, FilePathBuf, 0);
        if (0 == Length || MAX_PATH < Length)
            FilePathBuf[0] = '\0';
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
    {
        if (XSYM(BangShouldExecute)(FilePathBuf))
            XSYM(BangExecute)(CreateProcessPacket, FilePathBuf, 0);
        /* do not detour if we are executing an interpreter script to avoid recursive execution */
        CreateProcessPacket->Detour =
            CreateProcessPacket->lpApplicationName != CreateProcessPacket->ApplicationName &&
                XSYM(BangShouldDetour)(FilePathBuf);
    }
}

VOID XSYM(BangBeforeShellExecute)(XSYM(SHELLEXECUTEINFO) *ShellExecuteInfo)
{
    static XTYP ClassName[] = XLIT(".exe");
    XTYP FilePathBuf[MAX_PATH];
    DWORD Length;
    BOOL IsExecutable;

    if (sizeof *ShellExecuteInfo == ShellExecuteInfo->cbSize &&
        (0 == (ShellExecuteInfo->fMask & ~(
            SEE_MASK_NOCLOSEPROCESS |
            SEE_MASK_NOASYNC |
            SEE_MASK_DOENVSUBST |
            SEE_MASK_FLAG_NO_UI |
            SEE_MASK_UNICODE |
            SEE_MASK_NO_CONSOLE |
            SEE_MASK_ASYNCOK))) &&
        (0 == ShellExecuteInfo->lpVerb ||
            0 == XSTR(cmp)(XLIT("open"), ShellExecuteInfo->lpVerb) ||
            0 == XSTR(cmp)(XLIT("runas"), ShellExecuteInfo->lpVerb) ||
            0 == XSTR(cmp)(XLIT("runasuser"), ShellExecuteInfo->lpVerb)) &&
        0 != ShellExecuteInfo->lpFile)
    {
        Length = XSYM(GetFullPathName)(
            ShellExecuteInfo->lpFile, MAX_PATH, FilePathBuf, 0);
        if (0 == Length || MAX_PATH < Length)
            ;
        else if (XSYM(BangShouldExecute)(FilePathBuf))
        {
            XSYM(BangExecute)(0, FilePathBuf, &IsExecutable);
            if (IsExecutable)
            {
                ShellExecuteInfo->fMask |= SEE_MASK_CLASSNAME | SEE_MASK_FLAG_NO_UI | SEE_MASK_NO_CONSOLE;
                ShellExecuteInfo->lpClass = ClassName;
            }
        }
    }
}
