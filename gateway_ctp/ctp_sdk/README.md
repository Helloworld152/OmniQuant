# CTP SDK Setup Guide

为了编译真实网关，你需要下载 **Linux 64位** 版本的 CTP SDK (建议版本 v6.6.9 或 v6.7.2，适配 SimNow)。

## 1. 下载地址
- 上期技术官网: http://www.sfit.com.cn/
- 或者 SimNow 官网: https://www.simnow.com.cn/

## 2. 文件放置
请将下载包中的文件解压并放入以下目录：

### 头文件 (.h) -> `gateway_ctp/ctp_sdk/include/`
- ThostFtdcMdApi.h
- ThostFtdcTraderApi.h
- ThostFtdcUserApiDataType.h
- ThostFtdcUserApiStruct.h

### 库文件 (.so) -> `gateway_ctp/ctp_sdk/lib/`
- libthostmduserapi_se.so (行情库)
- libthosttraderapi_se.so (交易库)

> **注意**: 如果下载的文件名不同（例如带版本号），请重命名为上述标准名称，或者修改 CMakeLists.txt 适配。
