// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include <cstdio>
#include <stdio.h>
#include <string>
#include <windows.h>
#include <fstream>
#include <cstdlib>
#include <mutex>
 
#include "common.h"
#include "utils/logger.h"
#include "utils/ini_reader.h"

std::string config_file_dir;
const char *kDefaultIni =
    R"ini(
[log]
mode=disable
level=1
file=saveAsCSV.log

[export]
csv_dir=
append=0
symbol_width=6
symbol_suffix=XX
tz=+08
default_time=150000
)ini";


BOOL APIENTRY DllMain(HMODULE hModule,
                      DWORD reason,
                      LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        char ddl_path[MAX_PATH];
        GetModuleFileNameA(hModule, ddl_path, MAX_PATH);
        std::string path(ddl_path);
        size_t p = path.find_last_of("\\/");
        config_file_dir.reserve(MAX_PATH);
        config_file_dir.assign((p == std::string::npos) ? path : path.substr(0, p));
        Logger::initialize(config_file_dir);
    }
    else if (reason == DLL_PROCESS_DETACH)
    {
        Logger::shutdown();
    }
    return TRUE;
}
