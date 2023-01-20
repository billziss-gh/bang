<h1 align="center">
<img src="art/bang.png" width="256"/>
<br/>
Bang - #! script execution for Windows
</h1>


Bang adds to Windows programs the ability to execute interpreter scripts. Such scripts start with the `#!` character sequence, sometimes referred to as "hash-bang" or "she-bang".

A demonstration follows:

- Create a simple Python script:

    ```
    billziss@xps ⟩ ~ ⟩ Set-Content -Encoding ascii prargs.test

    cmdlet Set-Content at command pipeline position 1
    Supply values for the following parameters:
    Value[0]: #!/usr/bin/env python
    Value[1]: import sys; print(sys.argv)
    Value[2]:
    ```

- Attempt to execute:

    ```
    billziss@xps ⟩ ~ ⟩ Start-Process .\prargs.test "10 20" -NoNewWindow -Wait
    Start-Process : This command cannot be run due to the error: %1 is not a valid Win32 application.
    At line:1 char:1
    + Start-Process .\prargs.test "10 20" -NoNewWindow -Wait
    + ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        + CategoryInfo          : InvalidOperation: (:) [Start-Process], InvalidOperationException
        + FullyQualifiedErrorId : InvalidOperationException,Microsoft.PowerShell.Commands.StartProcess
    Command
    ```

- Start Bang and now it works:

    ```
    billziss@xps ⟩ ~ ⟩ .\Projects\bang\build\VStudio\build\Release\bang64.exe -p -1
    billziss@xps ⟩ ~ ⟩ Start-Process .\prargs.test "10 20" -NoNewWindow -Wait
    ['C:\\Users\\billziss\\prargs.test', '10', '20']
    ```

- Works in child processes as well:

    ```
    billziss@xps ⟩ ~ ⟩ cmd
    Microsoft Windows [Version 10.0.22621.963]
    (c) Microsoft Corporation. All rights reserved.

    C:\Users\billziss>.\prargs.test 10 20
    ['.\\prargs.test', '10', '20']
    ```

## How to use

From a shell prompt execute the command `bang64 -p -1` (or `bang32` on a 32-bit system). Your shell (and its children) will now have the "Bang" ability to execute interpreter scripts!

Bang is a command line utility with the following usage:

```
usage:
  bang [-p -1|PID]
  bang COMMAND [ARG ...]
```

- The `-p PID` option is used to add the Bang ability to an existing process with ID `PID`. `PID` can be `-1` in which case the ability is added to Bang's parent process and any new processes started from that process.

- Bang can also be used to start a new process `COMMAND` with arguments `ARG ...`. The new process and any processes started from that process will have the Bang ability.

## Script magic

Bang supports two different character sequences for interpreter scripts:

- **`#!/`**: This is the standard hash-bang sequence from Unix. For example:

    ```
    #!/usr/bin/env python
    import sys; print(sys.argv)
    ```

    A command line `.\script arguments-from-command-line` for a script with an interpreter line of `#!/interpreter optional-arguments`, results in executing `/interpreter` with the command line `.\script optional-arguments arguments-from-command-line`.

- **`///`**: Languages such as C use `//` for line comments. Furthermore Unix shells will often attempt to interpret a file as a shell script if the file does not contain an executable header or the `#!` sequence. Sometimes this is used to make a C/C++/etc. program executable:

    ```
    ///usr/bin/env gcc "$0"; exit
    int main() { return 0; }
    ```

    (The 3-slash convention originated on Unix-like environments (like Cygwin) that treat 2 slashes specially.)

    Bang supports a very limited form of this syntax. It does not understand any of the Unix shell syntax although it knows to strip the terminating `; exit`. It also supports variables `$0`-`$9` and the `$@` and `"$@"` incantations with a meaning similar (but not the same) to the one in the Unix shell. (In particular note that Bang treats quotes and backslashes as normal characters, except for the special case of `"$@"`.)

Bang uses a "Pathmap" to translate Unix-like interpreter paths to Windows-like interpreter paths. This Pathmap is configurable (see the Configuration section) but there is an internal default "Pathmap" that provides reasonable defaults:

- `/usr/bin/env COMMAND`: Instructs Bang to look for `COMMAND` in the `PATH`.

- `/usr/bin/COMMAND`: Instructs Bang to look for `COMMAND` in the `%SYSTEMROOT%\System32` directory (usually `C:\Windows\system32`).

## Configuration

Bang can be configured via the registry key `HKEY_CURRENT_USER\Software\Bang`. (The `HKEY_CURRENT_USER` key is specific to a particular user.) The following settings are available. All settings are of type "String" (`REG_SZ`) or "Expandable String" (`REG_EXPAND_SZ`).

- `Pathmap`: Controls the mapping of Unix-like paths to Windows-like paths. The syntax is `UnixPath*[WindowsPath][;...]` where `UnixPath` is a Unix-like path (e.g. `/usr/bin/`) and `WindowsPath` is a Windows-like path (e.g. `C:\Windows\System32\`).

    - The mappings specified in the Pathmap are attempted in order.

    - If `WindowsPath` is missing then searches are performed according to the `PATH` environment variable.

    - If `UnixPath` ends in slash `/` then it refers to a directory and matches against it are performed using prefix-matching. In this case `WindowsPath` must end in a backslash `\` and the `UnixPath` will be replaced by the `WindowsPath`. For example, the Pathmap `/usr/bin/*C:\Windows\System32\` will map `/usr/bin/cmd` to `C:\Windows\System32\cmd.exe` and the Pathmap `/usr/bin/*` will map `/usr/bin/cmd` to the `cmd.exe` file found by a `PATH` search.

    - If `UnixPath` does not end in slash then behavior depends: - If `WindowsPath` is missing then Bang will use the first optional argument in the intepreter line as the program to find by a `PATH` search. For example, the Pathmap `/usr/bin/env*` will map the interpreter line `#!/usr/bin/env cmd` to the `cmd.exe` file found by a `PATH` search. - If `WindowsPath` is present then Bang will simply substitute the `WindowsPath` in place of the `UnixPath`. For example, the Pathmap `/usr/bin/cmd*C:\Windows\System32\cmd.exe` will perform the obvious substitution.

    - The internal default Pathmap is the following:

        ```
        /usr/bin/env*;/usr/bin/*%SYSTEMROOT%\System32\;/bin/*%SYSTEMROOT%\System32\
        ```

- `Directories`: Controls the directory trees within which interpreter scripts must reside in order to be executable by Bang. The syntax is: `WindowsPath[;...]` and any specified `WindowsPath` must end in a backslash `\`. If the `Directories` setting is missing (the default) all scripts are executable by Bang regardless of location.

    - For example, the following setting will allow execution of scripts from the `.bin` and `Projects` directory subtrees only:

        ```
        %USERPROFILE%\.bin\;%USERPROFILE%\Projects\
        ```

- `Programs`: Controls the programs that can inherit the Bang ability. The syntax is: `WindowsPath[;...]`. The specified `WindowsPath` may be a fully qualified path such as `C:\Windows\System32\cmd.exe` or it may be a base file name such as `cmd.exe`. If the `Programs` setting is missing (the default) all programs can inherit the Bang ability.

    - For example, the following setting will allow only `cmd.exe` and `powershell.exe` to inherit the Bang ability:

        ```
        C:\Windows\System32\cmd.exe;powershell.exe
        ```

## How it works

Bang consists of a command line utility (EXE) and a dynamic link library (DLL). When Bang "injects" its DLL into a process, that process acquires the Bang ability. Any new process started from a process that has the Bang ability is also bestowed the Bang ability (but see the Configuration section for how to control which processes inherit the Bang ability).

The Bang DLL uses the Microsoft [Detours](https://github.com/microsoft/Detours) library to intercept calls to the `CreateProcess` and `ShellExecute` API's, which are used to create new processes on Windows. When Bang receives an intercepted `CreateProcess` call, it examines the executed file to see if it starts with one of the character sequences `#!/` or `///`. If either sequence is found, then the file is treated as an interpreter script and the `CreateProcess` call is altered accordingly.

## Security

There are two main security concerns:

- Bang enables any text file to become executable by simply adding the `#!/` or `///` script magic. To mitigate this risk use the `Directories` configuration setting to control the directory trees where scripts can reside.

- Bang injects its DLL into other processes. This DLL represents foreign code to these processes and has an associated risk. To mitigate this risk use the `Programs` configuration setting to control which processes inherit the Bang ability.

## Limitations

Although a process with the Bang ability can execute interpreter scripts, some processes do not always know what to do with their newfound ability.

For example, Powershell with the Bang ability does not know that it is able to execute non-EXE files. Consequently it treats them as document files and attempts to open them via `ShellExecute`. This still works because Bang also intercepts `ShellExecute`, but the experience is not quite the same as with native programs.

```
billziss@xps ⟩ ~ ⟩ .\prargs.test | sort
Cannot run a document in the middle of a pipeline: C:\Users\billziss\prargs.test.
At line:1 char:1
+ .\prargs.test | sort
+ ~~~~~~~~~~~~~
    + CategoryInfo          : InvalidOperation: (C:\Users\billziss\prargs.test:String) [], Runtime
   Exception
    + FullyQualifiedErrorId : CantActivateDocumentInPipeline

billziss@xps ⟩ ~ ⟩ Start-Process .\prargs.test -NoNewWindow -Wait | sort
['C:\\Users\\billziss\\prargs.test']
```

There is work-in-progress to improve this experience. One trick that works is to place your script's extension in the `PATHEXT` environment variable or to simply name your script with a `.exe` extension!

```
billziss@xps ⟩ ~ ⟩ mv .\prargs.test .\prargs.exe
billziss@xps ⟩ ~ ⟩ .\prargs.exe | sort
['C:\\Users\\billziss\\prargs.exe']
```
