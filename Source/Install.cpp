// SPDX-License-Identifier: GPL-3.0-only
// Runs Plugweaver's install script with administrator rights, using the
// system's own approval dialog:
//   macOS:   the standard password dialog (AppleScript "with administrator
//            privileges")
//   Windows: the UAC prompt (ShellExecuteEx "runas")
//   Linux:   not used; plugins go to the user's own ~/.vst3 and ~/.clap
#include "Model.h"
#if JUCE_WINDOWS
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shellapi.h>
#endif

namespace pw {
bool runPrivilegedScript(const File &script, const String &reason,
                         String &errorMessage) {
#if JUCE_MAC
  auto appleScriptString = [](const String &s) {
    return "\"" + s.replace("\\", "\\\\").replace("\"", "\\\"") + "\"";
  };
  String command = "do shell script \"/bin/sh \" & quoted form of " +
                   appleScriptString(script.getFullPathName()) +
                   " with prompt " + appleScriptString(reason) +
                   " with administrator privileges";
  ChildProcess process;
  if (!process.start(StringArray{"/usr/bin/osascript", "-e", command},
                     ChildProcess::wantStdOut | ChildProcess::wantStdErr)) {
    errorMessage = "Could not ask for administrator approval";
    return false;
  }
  auto output = process.readAllProcessOutput();
  process.waitForProcessToFinish(-1);
  if (process.getExitCode() == 0)
    return true;
  errorMessage = output.contains("-128")
                     ? "Installation cancelled - the plugin was not installed"
                     : "Installation failed: " + output.trim();
  return false;
#elif JUCE_WINDOWS
  ignoreUnused(reason);
  String parameters = "-NoProfile -NonInteractive -ExecutionPolicy Bypass "
                      "-File \"" +
                      script.getFullPathName() + "\"";
  SHELLEXECUTEINFOW info{};
  info.cbSize = sizeof(info);
  info.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NO_CONSOLE;
  info.lpVerb = L"runas";
  info.lpFile = L"powershell.exe";
  info.lpParameters = parameters.toWideCharPointer();
  info.nShow = SW_HIDE;
  if (!ShellExecuteExW(&info) || info.hProcess == nullptr) {
    errorMessage = GetLastError() == ERROR_CANCELLED
                       ? "Installation cancelled - the plugin was not installed"
                       : "Could not ask for administrator approval";
    return false;
  }
  WaitForSingleObject(info.hProcess, INFINITE);
  DWORD exitCode = 1;
  GetExitCodeProcess(info.hProcess, &exitCode);
  CloseHandle(info.hProcess);
  if (exitCode == 0)
    return true;
  errorMessage = "Installation failed. Close your music program and try again.";
  return false;
#else
  ignoreUnused(script, reason);
  errorMessage = "Cannot write to the plugin folder";
  return false;
#endif
}
} // namespace pw
