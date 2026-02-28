/****************************************************************************
 * SaveAsCSV TDX Export DLL
 * Copyright (C) 2026, GeanRain
 ****************************************************************************/

/*
 * TDX Input Mapping (confirmed)
 *
 * Func1: control/meta
 *   pIn1[0] cmd: 0 reset, 1 set symbol, 2 set period, 3 write csv
 *   cmd=1: pIn2[0]=symbol_numeric (numeric code, zero-padded to symbol_width)
 *          pIn3[0]=market_code
 *   cmd=2: pIn2[0]=period_code (TDX 0-13)
 *
 * Func2: date/time arrays
 *   pIn1[] = date (YYYYMMDD - 19000000)
 *   pIn2[] = time (HHMM, 0000-2359)
 *
 * Func3: OHLC by type
 *   pIn1[0] = type (1=open, 2=close, 3=high, 4=low)
 *   pIn2[]  = data array
 *
 * Period code mapping used:
 *   0=1m, 1=5m, 3=30m, 5=1d, 6=1w, 7=1mo, 11=1y
 */
#include "linkTDX.h"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include "../../common.h"
#include "../../utils/ini_reader.h"
#include "../../utils/logger.h"

extern std::string config_file_dir;

namespace
{
struct ExportState
{
    bool config_loaded = false;
    bool has_meta = false;
    bool has_period = false;
    bool has_date = false;
    bool has_time = false;
    bool has_open = false;
    bool has_high = false;
    bool has_low = false;
    bool has_close = false;

    int count = 0;
    int symbol_numeric = 0;
    int market_code = 0;
    int period_code = 0;

    std::vector<float> date;
    std::vector<float> time;
    std::vector<float> open;
    std::vector<float> high;
    std::vector<float> low;
    std::vector<float> close;

    std::string csv_dir;
    bool append = false;
    int symbol_width = 6;
    std::string symbol_suffix = "XX";
    std::string tz = "+08";
    int default_time = 150000;
};

ExportState g_state;

std::string join_path(const std::string &a, const std::string &b)
{
    if (a.empty())
        return b;
    if (a.back() == '\\' || a.back() == '/')
        return a + b;
    return a + "\\" + b;
}

bool path_is_relative(const std::string &p)
{
    if (p.empty())
        return true;
    if (p.size() >= 2 && p[1] == ':')
        return false;
    if (p.size() >= 2 && p[0] == '\\' && p[1] == '\\')
        return false;
    return true;
}

bool ensure_dir(const std::string &path)
{
    if (path.empty())
        return false;

    DWORD attr = GetFileAttributesA(path.c_str());
    if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY))
        return true;

    std::string current;
    current.reserve(path.size());
    for (size_t i = 0; i < path.size(); ++i)
    {
        char c = path[i];
        current.push_back(c);
        if (c == '\\' || c == '/')
        {
            CreateDirectoryA(current.c_str(), nullptr);
        }
    }
    return CreateDirectoryA(path.c_str(), nullptr) ||
           GetLastError() == ERROR_ALREADY_EXISTS;
}

// TDX period code mapping (0-13):
// 0=1m,1=5m,2=15m,3=30m,4=60m,5=1d,6=1w,7=1mo,8=multi-min,
// 9=multi-day,10=quarter,11=1y,12=5s,13=multi-sec
std::string period_code_to_str(int code)
{
    switch (code)
    {
    case 0:
        return "1m";
    case 1:
        return "5m";
    case 3:
        return "30m";
    case 5:
        return "1d";
    case 6:
        return "1w";
    case 7:
        return "1mo";
    case 11:
        return "1y";
    default:
        return "unknown";
    }
}

std::string market_code_to_suffix(int code)
{
    switch (code)
    {
    case 0:
        return "SZ";
    case 1:
        return "SH";
    case 2:
        return "BJ";
    case 47:
        return "CFFEX";
    case 28:
        return "CZCE";
    case 29:
        return "DCE";
    case 30:
        return "SHFE";
    case 27:
        return "HKI";
    case 31:
        return "HK";
    case 48:
        return "HKGEM";
    default:
        return "";
    }
}

std::string format_symbol(int symbol_numeric, int market_code, int width, const std::string &fallback_suffix)
{
    std::ostringstream oss;
    if (width > 0)
        oss << std::setw(width) << std::setfill('0') << symbol_numeric;
    else
        oss << symbol_numeric;

    std::string suffix = market_code_to_suffix(market_code);
    if (suffix.empty())
        suffix = fallback_suffix;
    if (!suffix.empty())
        return oss.str() + "." + suffix;
    return oss.str();
}

void split_date(int yyyymmdd, int &y, int &m, int &d)
{
    y = yyyymmdd / 10000;
    m = (yyyymmdd / 100) % 100;
    d = yyyymmdd % 100;
}

void split_time(int t, int &hh, int &mm, int &ss)
{
    if (t < 0)
        t = 0;
    if (t < 10000)
    {
        hh = t / 100;
        mm = t % 100;
        ss = 0;
        return;
    }
    hh = t / 10000;
    mm = (t / 100) % 100;
    ss = t % 100;
}

std::string format_ts(int date, int time, const std::string &tz)
{
    int y = 0, m = 0, d = 0;
    int hh = 0, mm = 0, ss = 0;
    split_date(date, y, m, d);
    split_time(time, hh, mm, ss);

    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(4) << y << "-"
        << std::setw(2) << m << "-" << std::setw(2) << d << " "
        << std::setw(2) << hh << ":" << std::setw(2) << mm << ":"
        << std::setw(2) << ss << tz;
    return oss.str();
}

std::string date_dir_name(int date)
{
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(8) << date;
    return oss.str();
}

void ensure_config_loaded()
{
    if (g_state.config_loaded)
        return;

    std::string iniPath = join_path(config_file_dir, INIREADER_FILENAME);
    IniReader ini(iniPath, kDefaultIni);

    g_state.csv_dir = ini.getString("export", "csv_dir", "");
    g_state.append = ini.getInt("export", "append", 0) != 0;
    g_state.symbol_width = ini.getInt("export", "symbol_width", 6);
    g_state.symbol_suffix = ini.getString("export", "symbol_suffix", "XX");
    g_state.tz = ini.getString("export", "tz", "+08");
    g_state.default_time = ini.getInt("export", "default_time", 150000);

    g_state.config_loaded = true;
}

void reset_state()
{
    g_state.has_meta = false;
    g_state.has_period = false;
    g_state.has_date = false;
    g_state.has_time = false;
    g_state.has_open = false;
    g_state.has_high = false;
    g_state.has_low = false;
    g_state.has_close = false;

    g_state.count = 0;
    g_state.symbol_numeric = 0;
    g_state.market_code = 0;
    g_state.period_code = 0;

    g_state.date.clear();
    g_state.time.clear();
    g_state.open.clear();
    g_state.high.clear();
    g_state.low.clear();
    g_state.close.clear();
}

void ensure_count(int nCount)
{
    if (nCount <= 0)
        return;

    if (g_state.count != nCount)
    {
        g_state.count = nCount;
        g_state.date.assign(nCount, 0);
        g_state.time.assign(nCount, 0);
        g_state.open.assign(nCount, 0);
        g_state.high.assign(nCount, 0);
        g_state.low.assign(nCount, 0);
        g_state.close.assign(nCount, 0);
    }
}

bool write_csv()
{
    ensure_config_loaded();

    if (!g_state.has_meta || !g_state.has_period || !g_state.has_date ||
        !g_state.has_open || !g_state.has_high || !g_state.has_low ||
        !g_state.has_close)
    {
        LOG_WARN("Export skipped: incomplete data (meta=%d period=%d date=%d ohlc=%d)",
                 g_state.has_meta, g_state.has_period, g_state.has_date,
                 (g_state.has_open && g_state.has_high && g_state.has_low && g_state.has_close));
        return false;
    }

    int date0 = 0;
    if (!g_state.date.empty())
        date0 = static_cast<int>(g_state.date[0]);

    std::string base_dir = g_state.csv_dir;
    if (base_dir.empty())
        base_dir = join_path(config_file_dir, "csv");
    else if (path_is_relative(base_dir))
        base_dir = join_path(config_file_dir, base_dir);

    if (!ensure_dir(base_dir))
    {
        LOG_ERROR("Export failed: cannot create directory: %s", base_dir.c_str());
        return false;
    }

    std::string period_str = period_code_to_str(g_state.period_code);
    std::string symbol_str = format_symbol(g_state.symbol_numeric,
                                            g_state.market_code,
                                            g_state.symbol_width,
                                            g_state.symbol_suffix);
    std::string file_path = join_path(base_dir, symbol_str + "." + period_str + ".csv");

    bool file_exists = (GetFileAttributesA(file_path.c_str()) != INVALID_FILE_ATTRIBUTES);
    std::ios::openmode mode = std::ios::out | std::ios::binary;
    if (g_state.append)
        mode |= std::ios::app;
    else
        mode |= std::ios::trunc;

    std::ofstream out(file_path, mode);
    if (!out.is_open())
    {
        LOG_ERROR("Export failed: cannot open file: %s", file_path.c_str());
        return false;
    }

    if (!g_state.append || !file_exists)
        out << "symbol,period,ts,open,high,low,close\n";

    for (int i = 0; i < g_state.count; ++i)
    {
        int date_raw = static_cast<int>(g_state.date[i]);
        int date = date_raw + 19000000;
        int time = (g_state.has_time && i < (int)g_state.time.size())
                       ? static_cast<int>(g_state.time[i])
                       : g_state.default_time;

        if (date <= 0)
            continue;

        std::string ts = format_ts(date, time, g_state.tz);
        out << symbol_str << ',' << period_str << ',' << ts << ','
            << g_state.open[i] << ',' << g_state.high[i] << ','
            << g_state.low[i] << ',' << g_state.close[i] << '\n';
    }

    out.flush();
    LOG_INFO("Export done: %s", file_path.c_str());
    return true;
}

void set_ohlc_by_type(int nCount, int type, const float *data)
{
    if (!data || nCount <= 0)
        return;

    switch (type)
    {
    case 1: // open
        std::copy(data, data + nCount, g_state.open.begin());
        g_state.has_open = true;
        break;
    case 2: // close
        std::copy(data, data + nCount, g_state.close.begin());
        g_state.has_close = true;
        break;
    case 3: // high
        std::copy(data, data + nCount, g_state.high.begin());
        g_state.has_high = true;
        break;
    case 4: // low
        std::copy(data, data + nCount, g_state.low.begin());
        g_state.has_low = true;
        break;
    default:
        break;
    }
}

} // namespace

// Func1: control & meta
// pIn1[0] cmd: 0 reset, 1 set symbol, 2 set period, 3 write csv
// cmd=1: pIn2[0]=symbol_numeric, pIn3[0]=market_code
// cmd=2: pIn2[0]=period_code (TDX 0-13)
void Func1(int nCount, float *pOut, float *pIn1, float *pIn2, float *pIn3)
{
    (void)pOut;
    if (!pIn1)
        return;

    int cmd = static_cast<int>(pIn1[0]);

    switch (cmd)
    {
    case 0:
        reset_state();
        LOG_INFO("Reset state");
        break;
    case 1:
        ensure_count(nCount);
        if (pIn2)
            g_state.symbol_numeric = static_cast<int>(pIn2[0]);
        if (pIn3)
            g_state.market_code = static_cast<int>(pIn3[0]);
        g_state.has_meta = true;
        LOG_INFO("Set symbol: code=%d market=%d", g_state.symbol_numeric, g_state.market_code);
        break;
    case 2:
        if (pIn2)
            g_state.period_code = static_cast<int>(pIn2[0]);
        g_state.has_period = true;
        LOG_INFO("Set period: code=%d", g_state.period_code);
        break;
    case 3:
        LOG_INFO("Write CSV requested");
        write_csv();
        break;
    default:
        break;
    }
}

// Func2: date/time arrays
// pIn1 = date (YYYYMMDD - 19000000), pIn2 = time (HHMM)
void Func2(int nCount, float *pOut, float *pIn1, float *pIn2, float *pIn3)
{
    (void)pOut;
    (void)pIn3;
    ensure_count(nCount);
    if (pIn1)
    {
        std::copy(pIn1, pIn1 + nCount, g_state.date.begin());
        g_state.has_date = true;
        LOG_INFO("Date array received: n=%d", nCount);
    }
    if (pIn2)
    {
        std::copy(pIn2, pIn2 + nCount, g_state.time.begin());
        g_state.has_time = true;
        LOG_INFO("Time array received: n=%d", nCount);
    }
}

// Func3: data input by type
// pIn1[0] = type (1=open,2=close,3=high,4=low)
// pIn2 = data array
void Func3(int nCount, float *pOut, float *pIn1, float *pIn2, float *pIn3)
{
    (void)pOut;
    (void)pIn3;
    ensure_count(nCount);
    if (!pIn1)
        return;

    int type = static_cast<int>(pIn1[0]);
    set_ohlc_by_type(nCount, type, pIn2);
    LOG_INFO("OHLC array received: type=%d n=%d", type, nCount);
}

void Func4(int nCount, float *pOut, float *pIn1, float *pIn2, float *pIn3)
{
    (void)nCount;
    (void)pOut;
    (void)pIn1;
    (void)pIn2;
    (void)pIn3;
}

void Func5(int nCount, float *pOut, float *pIn1, float *pIn2, float *pIn3)
{
    (void)nCount;
    (void)pOut;
    (void)pIn1;
    (void)pIn2;
    (void)pIn3;
}

void Func6(int nCount, float *pOut, float *pIn1, float *pIn2, float *pIn3)
{
    (void)nCount;
    (void)pOut;
    (void)pIn1;
    (void)pIn2;
    (void)pIn3;
}

void Func7(int nCount, float *pOut, float *pIn1, float *pIn2, float *pIn3)
{
    (void)nCount;
    (void)pOut;
    (void)pIn1;
    (void)pIn2;
    (void)pIn3;
}

void Func8(int nCount, float *pOut, float *pIn1, float *pIn2, float *pIn3)
{
    (void)nCount;
    (void)pOut;
    (void)pIn1;
    (void)pIn2;
    (void)pIn3;
}

static PluginTCalcFuncInfo Info[] =
    {
        {1, &Func1},
        {2, &Func2},
        {3, &Func3},
        {4, &Func4},
        {5, &Func5},
        {6, &Func6},
        {7, &Func7},
        {8, &Func8},
        {0, NULL},
};

BOOL RegisterTdxFunc(PluginTCalcFuncInfo **pInfo)
{
    if (*pInfo == NULL)
    {
        *pInfo = Info;
        return TRUE;
    }
    return FALSE;
}
