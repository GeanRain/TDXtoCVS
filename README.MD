# SaveAsCSV - 通达信数据导出 DLL

将通达信（TDX）传入的行情数据导出为标准化 CSV 文件，供后续数据库导入与分析使用。

## 程序作用

- 接收通达信调用 DLL 时传入的浮点数组数据（日期、时间、OHLC）
- 统一格式化并输出 CSV 文件
- 支持市场类型与周期映射
- 输出路径与行为可通过配置文件控制

## 环境与依赖

- Windows
- Visual Studio（建议 2019 或更新）
- C++ 编译环境

## 目录结构（关键部分）

- `SaveAsCSV/link/tdx/linkTDX.cpp` 导出逻辑实现
- `SaveAsCSV/utils/logger.*` 日志系统
- `SaveAsCSV/utils/ini_reader.*` INI 读取
- `SaveAsCSV/dllmain.cpp` DLL 入口
- `docs/tdx_mapping.md` 通达信输入映射与导出规范
- `docs/design_doc_01.md` 程序设计文档

## 安装与编译

1. 使用 Visual Studio 打开工程解决方案。
2. 选择目标平台（x86 / x64）与编译配置（Debug / Release）。
3. 编译生成 DLL。
4. 将 DLL 以及生成的 `saveAsCVS.ini` 放置到通达信插件目录或你的指定目录。

## 配置文件

首次加载 DLL 会自动生成 `saveAsCVS.ini`，位于 DLL 同目录。关键配置项：

```
[export]
csv_dir=
append=0
symbol_width=6
symbol_suffix=XX
tz=+08
default_time=150000
```

- `csv_dir`：为空时默认输出到 `<dll_dir>\csv`
- `append`：1 追加写入，0 覆盖写入（推荐盘后全量导出）
- `symbol_width`：股票代码补零位数
- `symbol_suffix`：未知市场回退后缀
- `tz`：时间戳时区后缀
- `default_time`：无时间时使用的默认时间（如日线）

## 使用方式

### 调用顺序（通达信侧）

1. 绑定dll到通达信里
2. 新建条件选股公式，并设置**使用数据为全部**

```
F1:=TDXDLL1(1,0,0,0); {重置数据}
F2:=TDXDLL1(1,1,STR2CON(CODE),SETCODE); {设置代码和市场代号}
F3:=TDXDLL1(1,2,PERIOD,0); {设置周期}
F4:=TDXDLL1(2,DATE,TIME,0); {设置日期时间}
F5:=TDXDLL1(3,1,OPEN,0); {设置开盘价}
F6:=TDXDLL1(3,2,CLOSE,0); {设置收盘价}
F7:=TDXDLL1(3,3,HIGH,0); {设置最高价}
F8:=TDXDLL1(3,4,LOW,0); {设置最低价}
F9:=TDXDLL1(1,3,0,0); {开始保存}
F:1;

```

### 输出文件

- 文件名：`代码.市场.周期.csv`
- 示例：`600000.SH.1d.csv`
- CSV 表头：`symbol,period,ts,open,high,low,close`

## 文档

- 通达信输入映射：`docs/tdx_mapping.md`
- 程序设计文档：`docs/design_doc_01.md`

