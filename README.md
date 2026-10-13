# GD32H759IM USB 副屏与摄像头

基于 RT-Thread 和 GD32H7xx 的 USB 副屏方案，用 USB 高速口的 WinUSB 跟电脑通信，板载 LVGL 菜单切换三种模式：PC 状态仪表盘、电脑屏幕镜像、OV2640 摄像头画面推到电脑预览。

## 硬件平台

| 项 | 规格 |
|----|------|
| MCU | GD32H759IMK6，Cortex-M7 @ 480MHz，2MB Flash / 1MB SRAM |
| 显示 | 4.3 寸 480×272 RGB565 TFT，TLI 并行 RGB 接口 |
| 触摸 | GT911 电容触摸，软件 I2C（SCL=PH7，SDA=PF9，RST=PF6，INT=PH6） |
| 摄像头 | OV2640，DCI 8 位并行 + DMA1 通道 7，寄存器走硬件 I2C1（SDA=PB11，SCL=PH4） |
| SDRAM | 32MB @ 0xC0000000（EXMC），摄像头双缓冲放这里 |
| USB | 片内 USBHS0 高速设备 + 内置 PHY，WinUSB，VID=0x28E9 PID=0x028F |
| 用户键 | PA0，上升沿中断，任何模式下按它都退出 |
| 晶振 | HXTAL 24MHz，CKOUT0 三分频输出 8MHz 给 OV2640 当 XCLK |

## 工作模式

模式都在板载菜单上点，主机端把 [tools/usb2screen.py](tools/usb2screen.py) 跑起来就不用管了。

| 模式 | 说明 | 退出方式 |
|------|------|------|
| PC Monitor | 主机推送电脑状态仪表盘 | 触摸右上角 X / PA0 / 主机停发 100ms |
| Screen Mirror | 主机实时镜像电脑主屏 | 触摸右上角 X / PA0 / 连续 10 次 100ms 无数据 |
| USB Camera | OV2640 画面经 bulk IN 推到电脑预览 | 再点一次摄像头按钮 / PA0 |

摄像头模式不接管屏幕，板子仍停在菜单上。屏幕归属由一个 `scr_active` 标志切换，占用期间 LVGL 的 flush 和输入回调都直接让位，触摸改由 scr_ctl 独占轮询。

## 数据协议

屏幕数据：主机按 52224 字节一块写 bulk OUT，固件收到一块就 flush 到显存对应的偏移上，凑满一帧（480×272×2 = 261120 字节）计一次帧数。

模式包：固件把当前模式用一个字节从 bulk IN 发给主机，0 停止 / 1 监控 / 2 镜像 / 3 摄像头。模式变化或 USB 重新枚举后会补发两次。

摄像头帧：8 字节帧头加帧数据，都走 bulk IN。

- 帧头：`'C','A','M'` + 1 字节序号 + 4 字节小端长度
- 帧数据：320×240 RGB565，153600 字节
- 帧地址由采集回调通过信号量 `cam_frame_sem` 交给发送线程，帧头和数据两段串行发出

## 软件结构

RT-Thread 4.1.1，1ms 一个 tick，开了信号量、互斥量、事件集、邮箱、消息队列、内存池和软件定时器；设备框架留了串口和 PIN，MSH 控制台挂在 uart1；USB 用 RT-Thread 的 usbdevice 加 WinUSB 类。界面是 LVGL 9.5。

线程和优先级：

| 线程 | 优先级 | 职责 |
|------|-------|------|
| key | 9 | 按键事件线程，PA0 退出 |
| main | 10 | 拉起 key / usb2screen / fault_screen / lvgl_app |
| camera | 12 | 采集线程，等帧信号量后回调上层 |
| lvgl | 15 | lv_timer_handler 和页面切换 |
| scr_disp | 16 | 收 USB 显示流、写显存、判停顿 |
| scr_cam | 17 | 取帧并发送（帧头 + 数据） |
| scr_ctl | 18 | 发模式包、轮询触摸退出热区 |
| tshell | 20 | MSH 控制台 |

线程之间的 IPC：

- `scr_sem`：USB 接收回调唤醒 scr_disp
- `cam_frame_sem`：采集回调唤醒 scr_cam
- `cam_tx_sem`：USB 发送完成回调同步 scr_cam
- `scr_stats_mtx`：统计数据
- `scr_in_mtx`：USB IN 端点写互斥，scr_ctl 和 scr_cam 共用
- `sccb_mtx`：OV2640 寄存器读写互斥

显示通路用 LVGL 的部分渲染，draw buffer 开 80 行，flush 回调里忙等 TLI 的 line-mark 到位置再往显存拷，拷完清一次 D-Cache。摄像头通路是 DMA 双缓冲，两块分别在 SDRAM 的 0xC0000000 和 0xC0030000，中断里翻地址并释放信号量，线程读之前先 invalidate D-Cache。

## 目录结构

```
GD32/
├── applications/main.c          启动入口
├── BSP/                         自己写的板级代码
│   ├── CAMERA/                  OV2640、DCI+DMA、SCCB、SDRAM
│   ├── KEY/                     PA0 按键（中断加事件集）
│   ├── LCD/                     TLI 显存操作、HardFault 异常屏
│   ├── TOUCH/                   GT911 软件 I2C
│   ├── LVGL/                    显示/输入端口、板载菜单
│   └── USB2SCR/                 USB 副屏核心（三线程 + WinUSB）
├── drivers/                     GD32H7 板级驱动
├── libraries/                   GD32H7 标准外设库 + CMSIS
├── lvgl/                        LVGL 9.5 源码
├── rt-thread/                   RT-Thread 内核与组件
├── linkscripts/GD32H759IM/      链接脚本
├── tools/                       主机端工具
│   ├── usb2screen.py            三种模式的信号源和摄像头预览
│   ├── start_subdisplay.bat     双击启动主机工具
│   ├── install_winusb.ps1       WinUSB 驱动绑定（管理员）
│   ├── rtthread_winusb.inf      回退用的 INF
│   └── mirror_probe.png         镜像抓屏的调试截图
├── flash.bat                    一键烧录
└── makefile.targets             链接响应文件规则
```

## 编译与烧录

工程是 RT-Thread Studio（Eclipse CDT）建的，工具链 GNU Arm Embedded 13.3。

```powershell
$env:PATH = "D:\RT-ThreadStudio\repo\Extract\ToolChain_Support_Packages\ARM\GNU_Tools_for_ARM_Embedded_Processors\13.3\bin;D:\RT-ThreadStudio\platform\env_released\env\tools\bin;" + $env:PATH
cd D:\RT-ThreadStudio\workspace\GD32\Debug
make all
```

链接的对象有一千四百多个，直接拼命令行会超过 Windows 的 32KB 限制（报 Error 126），所以 [makefile.targets](makefile.targets) 把对象列表写进响应文件再传给 gcc。产物是 `Debug/rtthread.elf` 和 `Debug/rtthread.bin`。

烧录用 [flash.bat](flash.bat)，pyOCD 加 GD-Link，under-reset 连接配合整片擦除，避免旧固件跑起来抢 SWD：

```powershell
cd D:\RT-ThreadStudio\workspace\GD32
.\flash.bat
```

## 主机端使用

Python 3.7 以上，依赖一次装齐：

```powershell
pip install pyusb libusb psutil pillow numpy mss
```

第一次要在管理员 PowerShell 里绑定一次 WinUSB 驱动：

```powershell
cd D:\RT-ThreadStudio\workspace\GD32\tools
.\install_winusb.ps1
```

之后日常使用只要：

```powershell
.\tools\start_subdisplay.bat
```

也可以直接跑 `python tools\usb2screen.py`。工具默认是 host 模式，行为全交给板子菜单，USB 掉线会自己重连；另外留了 `--mode pattern / monitor / mirror / image` 几个固定信号源的调试模式，方便单独验证链路。

## 调试

硬故障不用连调试器：[fault_screen.c](BSP/LCD/fault_screen.c) 会把出错线程名、是否在中断里，以及 PC、LR、PSR、CFSR、HFSR、BFAR 打到屏上。

常见问题：

| 现象 | 原因 | 处理 |
|------|------|------|
| 设备管理器黄色感叹号（Code 28） | 设备节点被缓存过，固件回的微软 OS 描述符没被重新问一遍 | 运行 `tools\install_winusb.ps1` |
| 镜像画面倾斜（撕裂） | TLI 等扫描线那段用了 `rt_thread_mdelay` | 改回忙等 |
| 花屏，或者画面停在旧帧 | DMA 直写内存，缓存没做维护 | 读之前 invalidate，写完 clean |
| 链接报 Error 126 | 链接命令行超过 32KB | 用 `makefile.targets` 里的响应文件 |
| 触摸没反应 | GT911 初始化失败，I2C 上没有 ACK | 串口会打印 `GT911 init failed`，查引脚和上拉 |
| 烧录报 No ACK | 旧固件跑起来抢了 SWD | 用 under-reset 连接，或者拉高 BOOT0 重新上电 |

## 几条约束

- SDRAM 地址必须在 0xC0000000–0xC1FFFFFF 之间，一共 32MB。
- TLI 等 line-mark 只能忙等。
- LVGL 的页面容器 radius、pad、border 全设 0，控件用 `lv_obj_set_pos` 显式定位，不用 flex/grid，否则边缘会被裁掉。
- 工程里没有文件系统，画面全部走内存，不接 SD 卡和外挂 Flash。
