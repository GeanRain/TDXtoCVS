# 通达信 DLL 调用顺序

本文定义通达信侧调用 DLL 的推荐顺序与职责，用于稳定导出 CSV。

## 推荐调用顺序（每个 symbol + period）

1. `Func1(cmd=0)` 重置内部状态
2. `Func1(cmd=1)` 设置代码与市场类型
3. `Func1(cmd=2)` 设置周期
4. `Func2` 传入日期/时间数组
5. `Func3` 按类型传入 OHLC + 成交量 + 成交额（共 6 次）
6. `Func1(cmd=3)` 写 CSV

## 通达信公式示例（TDXDLL1）

### 1) Reset

```
TDXDLL1(1,0,0,0);
```

### 2) Set Symbol + Market

```
TDXDLL1(1,1,STR2CON(CODE),SETCODE);
```

- `CODE` 为通达信代码
- `SETCODE` 为市场类型码（见下表）

### 3) Set Period

```
TDXDLL1(1,2,PERIOD,0);
```

### 4) Set Date/Time

```
TDXDLL1(2,DATE,TIME,0);
```

### 5) Set OHLC/Volume/Amount by Type

```
TDXDLL1(3,1,OPEN,0);
TDXDLL1(3,2,CLOSE,0);
TDXDLL1(3,3,HIGH,0);
TDXDLL1(3,4,LOW,0);
TDXDLL1(3,5,VOL,0);
TDXDLL1(3,6,AMOUNT,0);
```

### 6) Write CSV

```
TDXDLL1(1,3,0,0);
```

## 市场类型码映射

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

## 周期码映射（使用部分）

- `0` -> `1m`
- `1` -> `5m`
- `3` -> `30m`
- `5` -> `1d`
- `6` -> `1w`
- `7` -> `1mo`
- `11` -> `1y`

## 注意事项

- 所有输入数组类型为 `float`，长度由 `nCount` 决定。
- `VOL` 单位为“手”，`AMOUNT` 单位为“元”。
- CSV 输出路径与格式由 `saveAsCVS.ini` 的 `[export]` 控制。
- CSV 表头为 `symbol,period,ts,open,high,low,close,volume,amount`。
- 导出模式自动识别：无同名文件按全量写入；有同名文件按 `ts` 增量合并。
- 合并时若同 `ts` 数据不同，则覆盖旧数据；最终文件按 `ts` 升序写回。
- 输出文件名格式：`代码.市场.周期.csv`，如 `600000.SH.1d.csv`。
- 缺少必要字段时会跳过写入并记录日志。
