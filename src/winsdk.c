#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>

#pragma comment(linker, "/entry:start")
#pragma comment(linker, "/subsystem:console")

wchar_t const usage[] =
L"winsdk\n"
L"   --kit:{um, ucrt}\n"
L"   --type:{include,lib}\n"
L"   [--arch:{x86, x64, arm, arm64}]\n"
L"   [--version:10.0.00000.0]\n"
L"   --arch is required if --type:lib";

wchar_t const regKey[] =
   L"SOFTWARE\\WOW6432Node\\Microsoft\\Microsoft SDKs\\Windows\\v10.0";

wchar_t const regVal[] =
   L"InstallationFolder";

#define QWORD unsigned long long
#define DWORD_PTR *(DWORD *)
#define QWORD_PTR *(size_t *)

void start(void) {
   HANDLE const hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
   if (hStdout == INVALID_HANDLE_VALUE) {
      goto BAD_END_WINDOWS;
   }

   int argc;
   wchar_t const *const *restrict argv = (wchar_t const **)
      CommandLineToArgvW(GetCommandLineW(), &argc);

   // only --kit and --type are needed
   if (argc < 3 || argc > 5) {
      goto BAD_END_USAGE;
   }

   PCWSTR kit  = NULL;
   PCWSTR type = NULL;
   PCWSTR arch = NULL;
   PCWSTR version = NULL;

   for (wchar_t const *restrict arg; (arg = *++argv);) {
      // all command line arguments should start with "--"
      if (DWORD_PTR arg != DWORD_PTR L"--") {
         goto BAD_END_USAGE;
      }
      arg += 2;

      // --kit:
      if (QWORD_PTR arg == QWORD_PTR L"kit:") {
         if (kit) {
            goto BAD_END_USAGE;
         }
         kit = arg += 4;

         // all valid options begin with "u"
         if (*arg++ != L'u') {
            goto BAD_END_USAGE;
         }

         // --kit:um
         if (DWORD_PTR (arg + 0) == DWORD_PTR L"m") {
            continue;
         }

         // --kit:ucrt
         if (QWORD_PTR (arg + 0) == QWORD_PTR L"crt") {
            continue;
         }

         /*
         // --kit:ucrt_enclave
         if (1
            && QWORD_PTR (arg + 0) == QWORD_PTR L"crt_"
            && QWORD_PTR (arg + 4) == QWORD_PTR L"encl"
            && QWORD_PTR (arg + 8) == QWORD_PTR L"ave"
         ) {
            continue;
         }
         */

         goto BAD_END_USAGE;
      }

      // --????:
      if (arg[4] == L':') {
         // --type:
         if (QWORD_PTR arg == QWORD_PTR L"type") {
            if (type) {
               goto BAD_END_USAGE;
            }
            type = arg + 5;

            // --type:lib
            if (QWORD_PTR type == QWORD_PTR L"lib") {
               continue;
            }

            // --type:include
            if (1
               && QWORD_PTR (type + 0) == QWORD_PTR L"incl"
               && QWORD_PTR (type + 4) == QWORD_PTR L"ude"
            ) {
               continue;
            }

            goto BAD_END_USAGE;
         }

         // --arch:
         if (QWORD_PTR arg == QWORD_PTR L"arch") {
            if (arch) {
               goto BAD_END_USAGE;
            }
            arch = arg += 5;

            // --arch:x86
            if (QWORD_PTR arch == QWORD_PTR L"x86") {
               continue;
            }

            // --arch:x64
            if (QWORD_PTR arch == QWORD_PTR L"x64") {
               continue;
            }

            // --arch:arm
            if (QWORD_PTR arch == QWORD_PTR L"arm") {
               continue;
            }

            // --arch:arm64
            if (1
               && QWORD_PTR (arch + 0) == QWORD_PTR L"arm6"
               && DWORD_PTR (arch + 4) == DWORD_PTR L"4"
            ) {
               continue;
            }

            goto BAD_END_USAGE;
         }
      }

      // --version:
      if (1
         && QWORD_PTR (arg + 0) == QWORD_PTR L"vers"
         && QWORD_PTR (arg + 4) == QWORD_PTR L"ion:"
      ) {
         if (version) {
            goto BAD_END_USAGE;
         }
         version = arg + 8;
         continue;
      }

      goto BAD_END_USAGE;
   }

   DWORD regVal_sz;
   {
      LSTATUS res = RegGetValueW(
         HKEY_LOCAL_MACHINE,
         regKey,
         regVal,
         RRF_RT_REG_SZ,
         NULL,
         NULL,
         &regVal_sz
      );

      if (res != ERROR_SUCCESS) {
         goto BAD_END_WINDOWS;
      }
   }

   // eventually we are going to have something like
   // C:\Program Files (x86)\Windows Kits\10\include\ucrt\*
   size_t InstallationFolder_sz = 0
      + regVal_sz          // null byte used for path separator
      + sizeof(L"include") // longest --type option
      + sizeof(L"ucrt")    // longest --kit option
      + sizeof(L"*");

   char *const InstallationFolder = __builtin_alloca(InstallationFolder_sz);

   {
      LSTATUS res = RegGetValueW(
         HKEY_LOCAL_MACHINE,
         regKey,
         regVal,
         RRF_RT_REG_SZ,
         NULL,
         InstallationFolder,
         &regVal_sz
      );

      if (res != ERROR_SUCCESS) {
         goto BAD_END_WINDOWS;
      }
   }

   WriteConsoleW(
      hStdout,
      InstallationFolder,
      regVal_sz - 1,
      &regVal_sz,
      NULL
   );

   ExitProcess(0);
   __builtin_unreachable();

   BAD_END_USAGE:
   {
      WriteConsoleW(
         GetStdHandle(STD_ERROR_HANDLE),
         usage,
         sizeof(usage) / sizeof(WCHAR) - 1,
         (DWORD *) &hStdout,
         0
      );
      ExitProcess(1);
      __builtin_unreachable();
   }

   BAD_END_WINDOWS:
   {
      ExitProcess(GetLastError());
      __builtin_unreachable();
   }
}

// winsdkpath --type:lib --kit:um --arch:x86
// winsdkpath --type:include --kit:um --arch:x64
