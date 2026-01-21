# 全局变量导致的 Bug 分析

## 问题描述

在 `context.c` 中，使用全局变量传递线程入口函数和参数时，导致系统启动后无法正常进入 shell，出现卡死现象。

## 问题代码

原始实现使用全局变量存储线程入口函数和参数：

```c
static void (*tentry_)(void *) = NULL;
static void (*texit_)(void) = NULL;
static void *parameter_ = NULL;

void entry_wrapper(void *args) {
  tentry_(parameter_);
  texit_();
}

rt_uint8_t *rt_hw_stack_init(void *tentry, void *parameter, rt_uint8_t *stack_addr, void *texit) {
  rt_uint8_t *stk = (rt_uint8_t *)RT_ALIGN_DOWN((rt_ubase_t)stack_addr, sizeof(rt_ubase_t));
  stk -= sizeof(thread_info_t);
  stk = (rt_uint8_t *)RT_ALIGN_DOWN((rt_ubase_t)stk, sizeof(rt_ubase_t));
  thread_info_t *info = (thread_info_t *)stk;
  tentry_ = tentry;
  texit_ = texit;
  parameter_ = parameter;
  Context *ctx = kcontext((Area){ NULL, stk }, entry_wrapper, info);
  return (void *)ctx;
}
```

## 问题现象

系统启动后打印了初始信息，但一直无法进入 shell，与预期不符。通过 sdb 调试分析发现，系统一直在 idle 线程中死循环。

## 解决方案

根据讲义建议，将参数传递方式从全局变量改为使用栈存储，具体实现如下：

```c
typedef struct {
  void (*tentry)(void *);
  void (*texit)(void);
  void *parameter;
} thread_info_t;

void entry_wrapper(void *args) {
  thread_info_t *info = (thread_info_t *)args;
  info->tentry(info->parameter);
  info->texit();
}

rt_uint8_t *rt_hw_stack_init(void *tentry, void *parameter, rt_uint8_t *stack_addr, void *texit) {
  rt_uint8_t *stk = (rt_uint8_t *)RT_ALIGN_DOWN((rt_ubase_t)stack_addr, sizeof(rt_ubase_t));
  stk -= sizeof(thread_info_t);
  stk = (rt_uint8_t *)RT_ALIGN_DOWN((rt_ubase_t)stk, sizeof(rt_ubase_t));
  thread_info_t *info = (thread_info_t *)stk;
  info->tentry = tentry;
  info->texit = texit;
  info->parameter = parameter;
  Context *ctx = kcontext((Area){ NULL, stk }, entry_wrapper, info);
  return (void *)ctx;
}
```

修改后系统能够正常进入 shell。

## 根本原因

分析系统初始化流程，发现问题根源如下：

线程创建顺序为：`application` → `timer` → `idle`。由于使用全局变量 `tentry_`、`texit_`、`parameter_` 存储线程信息，每次调用 `rt_hw_stack_init()` 都会覆盖之前的值。因此，最后创建的 idle 线程会覆盖全局变量，导致所有线程在调度启动后都执行 idle 线程的代码，从而无法进入 shell。

系统初始化流程如下：

```c
int rtthread_startup(void) {
    // ...
    rt_system_scheduler_init();
    rt_application_init();        // (1) 创建 application 线程
    rt_system_timer_thread_init(); // (2) 创建 timer 线程
    rt_thread_idle_init();        // (3) 创建 idle 线程（覆盖全局变量）
    rt_system_scheduler_start();  // (4) 开始调度
}
```

## 总结

使用全局变量在多线程环境下存在竞态条件，后创建的线程会覆盖先创建线程的参数。通过将参数存储在各自的栈空间中，每个线程都能正确获取自己的入口函数和参数，从而解决了该问题。
