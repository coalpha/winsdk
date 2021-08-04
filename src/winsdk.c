#include <wchar.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>

#pragma comment(linker, "/entry:start")
#pragma comment(linker, "/subsystem:console")

wchar_t const usage[] =
L"winsdk\n"
L"   --kit:{um, ucrt}\n"
L"   --type:{include,lib}\n"
L"   [--arch:{x86, x64, arm, arm64}]   (required if --type:lib)\n"
L"   [--version:10.0.00000.0]          (12 characters max)\n";

wchar_t const regKey[] =
   L"SOFTWARE\\WOW6432Node\\Microsoft\\Microsoft SDKs\\Windows\\v10.0";

wchar_t const regVal[] =
   L"InstallationFolder";

#define SDK_VERSION_LENGTH 12

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

   enum kit {kit_bottom, um, ucrt} kit = kit_bottom;
   enum type {type_bottom, include, lib} type = type_bottom;
   enum arch {arch_bottom, x86, x64, arm, arm64} arch = arch_bottom;
   wchar_t const *restrict version = NULL;

   for (wchar_t const *restrict arg; (arg = *++argv);) {
      // all command line arguments should start with "--"
      if (DWORD_PTR arg != DWORD_PTR L"--") {
         goto BAD_END_USAGE;
      }
      arg += 2;

      // --kit:
      if (QWORD_PTR arg == QWORD_PTR L"kit:") {
         if (kit != kit_bottom) {
            goto BAD_END_USAGE;
         }
         arg += 4;

         // all valid options begin with "u"
         if (*arg++ != L'u') {
            goto BAD_END_USAGE;
         }

         // --kit:um
         if (DWORD_PTR (arg + 0) == DWORD_PTR L"m") {
            kit = um;
            continue;
         }

         // --kit:ucrt
         if (QWORD_PTR (arg + 0) == QWORD_PTR L"crt") {
            kit = ucrt;
            continue;
         }

         goto BAD_END_USAGE;
      }

      // --????:
      if (arg[4] == L':') {
         // --type:
         if (QWORD_PTR arg == QWORD_PTR L"type") {
            if (type != type_bottom) {
               goto BAD_END_USAGE;
            }
            arg += 5;

            // --type:lib
            if (QWORD_PTR arg == QWORD_PTR L"lib") {
               type = lib;
               continue;
            }

            // --type:include
            if (1
               && QWORD_PTR (arg + 0) == QWORD_PTR L"incl"
               && QWORD_PTR (arg + 4) == QWORD_PTR L"ude"
            ) {
               type = include;
               continue;
            }

            goto BAD_END_USAGE;
         }

         // --arch:
         if (QWORD_PTR arg == QWORD_PTR L"arch") {
            if (arch != arch_bottom) {
               goto BAD_END_USAGE;
            }
            arg += 5;

            // --arch:x86
            if (QWORD_PTR arg == QWORD_PTR L"x86") {
               arch = x86;
               continue;
            }

            // --arch:x64
            if (QWORD_PTR arg == QWORD_PTR L"x64") {
               arch = x64;
               continue;
            }

            // --arch:arm
            if (QWORD_PTR arg == QWORD_PTR L"arm") {
               arch = arm;
               continue;
            }

            // --arch:arm64
            if (1
               && QWORD_PTR (arg + 0) == QWORD_PTR L"arm6"
               && DWORD_PTR (arg + 4) == DWORD_PTR L"4"
            ) {
               arch = arm64;
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

   if (type == lib && arch == arch_bottom) {
      goto BAD_END_USAGE;
   }

   // eventually we are going to have something like
   // C:\Program Files (x86)\Windows Kits\10\include\10.0.00000.0\ucrt
   size_t InstallationFolder_sz = 0
      + regVal_sz          // null byte used for path separator
      + sizeof(L"lib")
      + sizeof(L"10.0.00000.0")
      + sizeof(L"ucrt")    // longest --kit option
      + sizeof(L"arm64")
      + 8;                 // extra 4 wchars in case or something

   wchar_t *const SdkPath = __builtin_alloca(InstallationFolder_sz);

   {
      LSTATUS res = RegGetValueW(
         HKEY_LOCAL_MACHINE,
         regKey,
         regVal,
         RRF_RT_REG_SZ,
         NULL,
         SdkPath,
         &regVal_sz
      );

      if (res != ERROR_SUCCESS) {
         goto BAD_END_WINDOWS;
      }
   }

   wchar_t *SdkPath_wh =
      SdkPath + (regVal_sz / sizeof(wchar_t) - 1);

   switch (type) {
   case include:
      QWORD_PTR SdkPath_wh = QWORD_PTR L"incl" ; SdkPath_wh += 4;
      QWORD_PTR SdkPath_wh = QWORD_PTR L"ude\\"; SdkPath_wh += 4;
      break;
   case lib:
      QWORD_PTR SdkPath_wh = QWORD_PTR L"lib\\"; SdkPath_wh += 4;
      break;
   case type_bottom:
      goto BAD_END_USAGE;
   };

   if (version) {
      size_t version_len = 0;
      while (*version) {
         *SdkPath_wh++ = *version++;
         version_len++;
      }

      if (version_len != SDK_VERSION_LENGTH) {
         goto BAD_END_USAGE;
      }
   } else {
      DWORD_PTR SdkPath_wh = DWORD_PTR L"*";

      WIN32_FIND_DATAW dir;
      HANDLE hFind = FindFirstFileW(SdkPath, &dir);

      if (hFind == INVALID_HANDLE_VALUE) {
         goto BAD_END_WINDOWS;
      }

      register QWORD currentVersion0 = 0;
      register QWORD currentVersion1 = 0;
      register QWORD currentVersion2 = 0;

      // find the latest version by "string" comparison
      while (FindNextFileW(hFind, &dir)) {
         {
            BYTE cFileName_len = 0;
            while (dir.cFileName[cFileName_len]) {
               cFileName_len++;
            }

            if (cFileName_len != SDK_VERSION_LENGTH) {
               continue;
            }
         }

         QWORD nextVersion0 = QWORD_PTR (dir.cFileName + 0);
         QWORD nextVersion1 = QWORD_PTR (dir.cFileName + 4);
         QWORD nextVersion2 = QWORD_PTR (dir.cFileName + 8);
         if (nextVersion0 < currentVersion0) {
            continue;
         }

         if (nextVersion0 > currentVersion0) {
            goto select_next_version;
         }

         if (nextVersion1 < currentVersion1) {
            continue;
         }

         if (nextVersion1 > currentVersion1) {
            goto select_next_version;
         }

         if (nextVersion2 <= currentVersion2) {
            continue;
         }

         select_next_version:
         currentVersion0 = nextVersion0;
         currentVersion1 = nextVersion1;
         currentVersion2 = nextVersion2;
      }

      QWORD_PTR SdkPath_wh = currentVersion0; SdkPath_wh += 4;
      QWORD_PTR SdkPath_wh = currentVersion1; SdkPath_wh += 4;
      QWORD_PTR SdkPath_wh = currentVersion2; SdkPath_wh += 4;
   }

   *SdkPath_wh++ = L'\\';
   switch (kit) {
   case kit_bottom:
      goto BAD_END_USAGE;
   case um:
      DWORD_PTR SdkPath_wh = DWORD_PTR L"um"  ; SdkPath_wh += 2;
      break;
   case ucrt:
      QWORD_PTR SdkPath_wh = QWORD_PTR L"ucrt"; SdkPath_wh += 4;
      break;
   }

   if (type == lib) {
      switch (arch) {
      case arch_bottom:
         goto BAD_END_USAGE;
      case x86:
         QWORD_PTR SdkPath_wh = QWORD_PTR L"\\x86"; SdkPath_wh += 4;
         break;
      case x64:
         QWORD_PTR SdkPath_wh = QWORD_PTR L"\\x64"; SdkPath_wh += 4;
         break;
      case arm:
         QWORD_PTR SdkPath_wh = QWORD_PTR L"\\arm"; SdkPath_wh += 4;
         break;
      case arm64:
         QWORD_PTR SdkPath_wh = QWORD_PTR L"\\arm"; SdkPath_wh += 4;
         DWORD_PTR SdkPath_wh = DWORD_PTR L"64"   ; SdkPath_wh += 2;
      }
   }

   DWORD miscDword;
   if (GetConsoleMode(hStdout, &miscDword)) {
      WriteConsoleW(
         hStdout,
         SdkPath,
         SdkPath_wh - SdkPath,
         &regVal_sz,
         NULL
      );
   } else {
      // convert from utf16 to ascii
      char *SdkPath_wh2 = (char *) SdkPath + 1;
      wchar_t *SdkPath_rh2 = SdkPath + 1;
      while (SdkPath_rh2 <= SdkPath_wh) {
         *SdkPath_wh2++ = *SdkPath_rh2++;
      }

      WriteFile(
         hStdout,
         SdkPath,
         SdkPath_wh2 - (char *) SdkPath - 1,
         &miscDword,
         NULL
      );
   }

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
