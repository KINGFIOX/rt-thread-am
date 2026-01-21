# integrate-am-apps.py 中 objcopy 操作详解

## 代码片段

```python
for obj in objs:
    os.system(f"{CROSS_COMPILE}objcopy --prefix-symbols=__am_{app_name}_ --set-section-flags .text*=readonly,noload --set-section-flags .*rodata*=readonly,noload {str(obj)}")
    os.system(f"{CROSS_COMPILE}objcopy --redefine-syms=redefine_sym.txt --prefix-alloc-sections=__am_apps {str(obj)}")
    os.system(f"{CROSS_COMPILE}objcopy --set-section-flags .text*=readonly,code,alloc --set-section-flags .*rodata*=readonly,data,alloc {str(obj)}")
```

## 三步操作详解

### 第一步：添加符号前缀 + 标记 section 为 noload

```bash
objcopy --prefix-symbols=__am_typing_game_ \
        --set-section-flags .text*=readonly,noload \
        --set-section-flags .*rodata*=readonly,noload \
        typing_game.o
```

| 参数                                            | 含义                                                   |
| ----------------------------------------------- | ------------------------------------------------------ |
| `--prefix-symbols=__am_typing_game_`            | 给**所有符号**加前缀：`main` → `__am_typing_game_main` |
| `--set-section-flags .text*=readonly,noload`    | 将 `.text` 段标记为 `noload`（暂时不加载到内存）       |
| `--set-section-flags .*rodata*=readonly,noload` | 将 `.rodata` 段也标记为 `noload`                       |

### 第二步：重定义符号 + 添加 section 前缀

```bash
objcopy --redefine-syms=redefine_sym.txt \
        --prefix-alloc-sections=__am_apps \
        typing_game.o
```

| 参数                                | 含义                                                          |
| ----------------------------------- | ------------------------------------------------------------- |
| `--redefine-syms=redefine_sym.txt`  | 根据文件内容重命名符号                                        |
| `--prefix-alloc-sections=__am_apps` | 给所有 allocatable section 加前缀：`.data` → `__am_apps.data` |

redefine_sym.txt 内容示例：
```
__am_typing_game_printf printf        # 库函数还原
__am_typing_game_ioe_init __dummy_ioe_init  # 初始化函数重定向到空函数
__am_typing_game_halt __rt_am_halt    # halt 重定向到 rt_thread_exit
```

### 第三步：恢复 section flags

```bash
objcopy --set-section-flags .text*=readonly,code,alloc \
        --set-section-flags .*rodata*=readonly,data,alloc \
        typing_game.o
```

| 参数                                                | 含义                        |
| --------------------------------------------------- | --------------------------- |
| `--set-section-flags .text*=readonly,code,alloc`    | 恢复 `.text` 为可执行代码段 |
| `--set-section-flags .*rodata*=readonly,data,alloc` | 恢复 `.rodata` 为只读数据段 |

## 为什么要先设 noload 再恢复 alloc？

这是一个巧妙的技巧！原因是 `--prefix-alloc-sections` 会影响**所有 allocatable 的 section**。

### 问题

如果直接运行第二步：

```bash
objcopy --prefix-alloc-sections=__am_apps typing_game.o
```

**所有** allocatable section 都会被加前缀：
- `.text` → `__am_apps.text` ❌ 不想要
- `.rodata` → `__am_apps.rodata` ❌ 不想要  
- `.data` → `__am_apps.data` ✅ 想要
- `.bss` → `__am_apps.bss` ✅ 想要

但我们只想给 `.data` 和 `.bss` 加前缀，不想改 `.text` 和 `.rodata`。

### 解决方案

利用 `--prefix-alloc-sections` **只影响 allocatable section** 的特性：

| 步骤   | 操作                  | .text         | .rodata       | .data              | .bss              |
| ------ | --------------------- | ------------- | ------------- | ------------------ | ----------------- |
| 原始   | -                     | alloc         | alloc         | alloc              | alloc             |
| 第一步 | 设为 noload           | **noload**    | **noload**    | alloc              | alloc             |
| 第二步 | prefix-alloc-sections | noload (跳过) | noload (跳过) | **__am_apps.data** | **__am_apps.bss** |
| 第三步 | 恢复 alloc            | **alloc**     | **alloc**     | __am_apps.data     | __am_apps.bss     |

## 为什么要将 .data 改为 __am_apps.data？

核心目的是**让多个 AM 应用的全局变量可以被统一管理和重置**。

### 改名后的内存布局

所有 AM 应用的数据段合并到 `__am_apps.data`：

```
链接后的内存布局:
0x80000000  .text (RT-Thread 代码)
0x80010000  .data (RT-Thread 全局变量)
0x80020000  __am_apps.data  ← 所有 AM 应用的全局变量都在这里！
            ├── counter (hello)
            ├── score, snake_len (snake)
            └── correct, wrong (typing_game)
0x80021000  __am_apps.bss   ← 所有 AM 应用的未初始化变量
```

链接脚本会生成边界符号：
```c
extern char __am_apps_data_start, __am_apps_data_end;
extern char __am_apps_bss_start, __am_apps_bss_end;
```

### 好处

在 `init.c` 中，每次运行 AM 应用前可以**一次性重置所有全局变量**：

```c
// 恢复 .data 段到初始值（从备份中）
memcpy(am_apps_data.start, am_apps_data_content, ...);
// 清零 .bss 段
memset(am_apps_bss.start, 0, ...);
```

**效果**：每次运行 `snake` 或 `typing_game`，全局变量都是初始状态，就像程序第一次启动一样。

## 完整符号变换流程

```
原始 typing-game.o:
  main, printf, memcpy, ioe_init, halt ...
         ↓
objcopy --prefix-symbols=__am_typing_game_
         ↓
  __am_typing_game_main
  __am_typing_game_printf
  __am_typing_game_memcpy
  __am_typing_game_ioe_init
  __am_typing_game_halt
         ↓
objcopy --redefine-syms=redefine_sym.txt
         ↓
  __am_typing_game_main     (保留，应用入口)
  printf                    (还原，用 RT-Thread 的)
  memcpy                    (还原，用 klib 的)
  __dummy_ioe_init          (重定向，空函数)
  __rt_am_halt              (重定向，调用 rt_thread_exit)
```

## Section 变换流程

```
原始 .o 文件
├── .text (code)           符号: main, foo, bar
├── .rodata (readonly data)
├── .data (initialized data)
└── .bss (uninitialized data)
              ↓
         第一步 objcopy
              ↓
├── .text (noload)         符号: __am_typing_game_main, __am_typing_game_foo, ...
├── .rodata (noload)
├── .data
└── .bss
              ↓
         第二步 objcopy
              ↓
├── .text (noload)         符号: __am_typing_game_main (保留)
├── .rodata (noload)              printf (还原)
├── __am_apps.data                __dummy_ioe_init (重定向)
└── __am_apps.bss
              ↓
         第三步 objcopy
              ↓
├── .text (code, alloc)    ← 恢复正常
├── .rodata (data, alloc)  ← 恢复正常
├── __am_apps.data
└── __am_apps.bss
```
