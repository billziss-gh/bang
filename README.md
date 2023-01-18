# Bang - #! script execution for Windows

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

- **`#!/`**: This is the standard hash-bang sequence from UNIX. For example:

    ```
    #!/usr/bin/env python
    import sys; print(sys.argv)
    ```

- **`///`**: Languages such as C use `//` for line comments. Furthermore UNIX shells will often attempt to interpret a file as a shell script if the file does not contain an executable header or the `#!` sequence. Sometimes this is used to make a C/C++/etc. program executable:

    ```
    ///usr/bin/env gcc "$0"; exit
    int main() { return 0; }
    ```

    (The 3-slash convention originated on UNIX-like environments (like Cygwin) that treat 2 slashes specially.)

Bang treats some of the UNIX-like interpreter paths specially:

- `/usr/bin/env COMMAND`: Instructs Bang to look in the `PATH` for `COMMAND` exactly as in a UNIX-like system.

- `/usr/bin/COMMAND`: Instructs Bang to look for `COMMAND` in the `%SYSTEMROOT%\System32` directory (usually `C:\Windows\system32`).

## How it works

Bang uses the Microsoft [Detours](https://github.com/microsoft/Detours) library to intercept calls to the `CreateProcess` API, which is used to create new processes on Windows. When Bang receives an intercepted `CreateProcess` call, it examines the executed file to see if it starts with one of the character sequences `#!/` or `///`. If either sequence is found, then the file is treated as an interpreter script and the `CreateProcess` call is altered accordingly.
