<#
    FreeBASIC Windows packaging
    ---------------------------

    File: windows-refresh-environment.ps1

    Purpose:

        Tell Explorer that the machine environment has changed so that new
        shells inherit installer PATH updates.

    Responsibilities:

        - Find the Explorer taskbar window.
        - Send a bounded WM_SETTINGCHANGE notification.
        - Remove the temporary copy of this script.

    This file intentionally does NOT contain:

        - Registry updates.
        - Installer or uninstaller logic.
        - FreeBASIC package selection.
#>

$ErrorActionPreference = 'SilentlyContinue'

try {
    $source = @'
using System;
using System.Runtime.InteropServices;

internal static class FreeBasicEnvironmentRefresh
{
    [DllImport("user32.dll", EntryPoint = "FindWindowW", CharSet = CharSet.Unicode)]
    internal static extern IntPtr FindWindow(string className, string windowName);

    [DllImport("user32.dll", EntryPoint = "SendMessageTimeoutW",
        CharSet = CharSet.Unicode, SetLastError = true)]
    internal static extern IntPtr SendMessageTimeout(
        IntPtr window,
        uint message,
        UIntPtr wparam,
        string lparam,
        uint flags,
        uint timeout,
        out UIntPtr result);
}
'@

    Add-Type -TypeDefinition $source

    $explorerWindow = [FreeBasicEnvironmentRefresh]::FindWindow(
        'Shell_TrayWnd',
        $null
    )

    if ($explorerWindow -ne [IntPtr]::Zero) {
        [UIntPtr]$result = [UIntPtr]::Zero

        # WM_SETTINGCHANGE is 0x001a. SMTO_ABORTIFHUNG is 0x0002.
        [void][FreeBasicEnvironmentRefresh]::SendMessageTimeout(
            $explorerWindow,
            0x001a,
            [UIntPtr]::Zero,
            'Environment',
            0x0002,
            1000,
            [ref]$result
        )
    }
}
finally {
    if ($PSCommandPath) {
        Remove-Item -LiteralPath $PSCommandPath -Force -ErrorAction SilentlyContinue
    }
}

# end of windows-refresh-environment.ps1
