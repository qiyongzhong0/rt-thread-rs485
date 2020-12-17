# RS485驱动包

## 1.简介

**rs485** rs485接口通信驱动包。

### 1.1目录结构

`rs485` 软件包目录结构如下所示：

``` 
rs485
├───inc                         // 头文件目录
│   └───rs485.h                 // API 接口头文件
├───src                         // 源码目录
│   |   rs485.c                 // 主模块
│   |   rs485_test.c            // 测试模块
│   |   rs485_sample_slave.c    // 从模式示例
│   └───rs485_sample_master.c   // 主模式示例
│   license                     // 软件包许可证
│   readme.md                   // 软件包使用说明
└───SConscript                  // RT-Thread 默认的构建脚本
```

### 1.2许可证

rs485 package 遵循 LGPLv2.1 许可，详见 `LICENSE` 文件。

### 1.3依赖

- RT_Thread 4.0
- serial
- pin

## 2.使用

### 2.1接口函数说明

#### rs485_inst_t * rs485_create(char *serial, int baudrate, int parity, int pin, int level);
- 功能 ：动态创建rs485实例
- 参数 ：serial--串口设备名称
- 参数 ：baudrate--串口波特率
- 参数 ：parity--串口检验位
- 参数 ：pin--rs485收发模式控制引脚
- 参数 ：level--发送模式控制电平
- 返回 ：成功返回实例指针，失败返回NULL

#### int rs485_destory(rs485_inst_t * hinst);
- 功能 ：销毁rs485实例
- 参数 ：hinst--rs485实例指针
- 返回 ：0--成功,其它--失败

#### int rs485_config(rs485_inst_t * hinst, int baudrate, int databits, int parity, int stopbits);
- 功能 ：配置rs485通信参数
- 参数 ：hinst--rs485实例指针
- 参数 ：baudrate--通信波特率
- 参数 ：databits--数据位数, 5~8
- 参数 ：parity--检验位, 0~2, 0--无校验, 1--奇校验, 2--偶校验
- 参数 ：stopbits--停止位, 0~1, 0--1个停止位, 1--2个停止位
- 返回 ：0--成功，其它--错误

#### int rs485_set_recv_tmo(rs485_inst_t * hinst, int tmo_ms);
- 功能 ：设置rs485接收超时时间
- 参数 ：hinst--rs485实例指针
- 参数 ：tmo_ms--超时时间,单位ms
- 返回 ：0--成功，其它--错误

#### int rs485_set_byte_tmo(rs485_inst_t * hinst, int tmo_ms);
- 功能 ：设置rs485接收字节间隔超时时间
- 参数 ：hinst--rs485实例指针
- 参数 ：tmo_ms--超时时间,单位ms
- 返回 ：0--成功，其它--错误

#### int rs485_connect(rs485_inst_t * hinst);
- 功能 ：打开rs485连接
- 参数 ：hinst--rs485实例指针
- 返回 ：0--成功，其它--错误

#### int rs485_disconn(rs485_inst_t * hinst);
- 功能 ：关闭rs485连接
- 参数 ：hinst--rs485实例指针
- 返回 ：0--成功，其它--错误

#### int rs485_recv(rs485_inst_t * hinst, void *buf, int size);
- 功能 ：从rs485接收数据
- 参数 ：hinst--rs485实例指针
- 参数 ：buf--接收数据缓冲区指针
- 参数 ：size--缓冲区尺寸
- 返回 ：>=0--接收到的数据长度，<0--错误

#### int rs485_send(rs485_inst_t * hinst, void *buf, int size);
- 功能 ：从rs485接收数据
- 参数 ：hinst--rs485实例指针
- 参数 ：buf--发送数据缓冲区指针
- 参数 ：size--发送数据长度
- 返回 ：>=0--发送的数据长度，<0--错误

#### int rs485_break_recv(rs485_inst_t * hinst);
- 功能 ：中断rs485接收等待
- 参数 ：hinst--rs485实例指针
- 返回 ：0--成功，其它--错误

### 2.2获取组件

- **方式1：**
通过 *Env配置工具* 或 *RT-Thread studio* 开启软件包，根据需要配置各项参数；配置路径为 *RT-Thread online packages -> peripherals packages -> rs485* 


### 2.3配置参数说明

| 参数宏 | 说明 |
| ---- | ---- |
| RS485_USING_TEST		| 使用测试功能
| RS485_TEST_SERIAL		| 串口设备名称
| RS485_TEST_BAUDRATE	| 串口波特率
| RS485_TEST_PARITY 	| 串口校验位
| RS485_TEST_PIN 		| 收发模式控制引脚
| RS485_TEST_LEVEL 		| 发送模式控制电平
| RS485_TEST_BUF_SIZE	| 缓冲区尺寸
| RS485_TEST_RECV_TMO 	| 接收超时时间

## 3. 联系方式

* 维护：qiyongzhong
* 主页：https://github.com/qiyongzhong0/rt-thread-rs485
* 邮箱：917768104@qq.com
