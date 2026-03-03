# TDX Input Mapping

This document defines the confirmed data interface between TongDaXin (TDX) and the DLL export layer.

## Function Overview

- `Func1` control + metadata
- `Func2` date/time arrays
- `Func3` price/amount data by type

## Func1: Control + Metadata

`pIn1[0]` is the command:

- `0` reset internal state
- `1` set symbol
- `2` set period
- `3` write CSV

When `cmd=1`:

- `pIn2[0] = symbol_numeric`
  - numeric symbol code (e.g. `600000`)
  - will be zero-padded to `symbol_width` (default `6`)
- `pIn3[0] = market_code`
  - numeric market code mapped to a standard suffix
  - final CSV symbol format: `000001.SH` (suffix falls back to `symbol_suffix` if unknown)

When `cmd=2`:

- `pIn2[0] = period_code`

## Func2: Date/Time Arrays

- `pIn1[] = date`
  - value format: `YYYYMMDD - 19000000`
  - example: `1000101` represents `2000-01-01`
- `pIn2[] = time`
  - value format: `HHMM` (range `0000-2359`)

## Func3: Price/Amount Data by Type

- `pIn1[0] = type`
- `pIn2[] = data array`

Type mapping:

- `1` open
- `2` close
- `3` high
- `4` low
- `5` volume (unit: hand)
- `6` amount (unit: CNY yuan)

## Period Code Mapping (TDX 0-13)

Only the required periods are used:

- `0` -> `1m`
- `1` -> `5m`
- `3` -> `30m`
- `5` -> `1d`
- `6` -> `1w`
- `7` -> `1mo`
- `11` -> `1y`

## Market Code Mapping (TDX)

The DLL maps TDX market codes to standard suffixes:

- `0` -> `SZ`
- `1` -> `SH`
- `2` -> `BJ`
- `47` -> `CFFEX`
- `28` -> `CZCE`
- `29` -> `DCE`
- `30` -> `SHFE`
- `27` -> `HKI`
- `31` -> `HK`
- `48` -> `HKGEM`

## CSV Output

CSV header:

```
symbol,period,ts,open,high,low,close,volume,amount
```

Merge behavior (auto full + incremental):

- if target CSV does not exist: full export from current input arrays
- if target CSV exists: merge by `ts`
- if same `ts` exists and values differ, incoming row overwrites existing row
- if `ts` does not exist, row is inserted
- final file is rewritten in ascending `ts` order

Default directory and formatting are controlled by `saveAsCVS.ini` under `[export]`.

### CSV file name

The file name uses:

```
<code>.<market>.<period>.csv
```

Example:

```
600000.SH.1d.csv
```

### Export config fields

- `append`
  - reserved field; export mode is auto-detected by file existence and merged by `ts`
- `tz`
  - timezone suffix in `ts`, default `+08`
- `default_time`
  - used when `time` is missing, default `150000`
