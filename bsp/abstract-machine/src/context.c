#include "rtdef.h"
#include <am.h>
#include <klib.h>
#include <rtthread.h>

static Context **from_sp_ptr = NULL;  // 保存当前上下文的位置
static Context **to_sp_ptr = NULL;    // 新上下文的位置
static bool need_sched = false;

// 这里相当于是 yield-os 的 scheduler.
static Context *ev_handler(Event e, Context *c) {
  switch (e.event) {
  case EVENT_YIELD:
    if (need_sched) {
      if (from_sp_ptr != NULL) { *from_sp_ptr = c; } // 保存当前线程的上下文
      c = *to_sp_ptr;
      need_sched = false;
    }
    break;
  default:
    printf("Unhandled event ID = %d\n", e.event);
    assert(0);
  }
  return c;
}

void __am_cte_init() { cte_init(ev_handler); }

// 只会在一开始调用
void rt_hw_context_switch_to(rt_ubase_t to) {
  from_sp_ptr = NULL;
  to_sp_ptr = (Context **)to;
  need_sched = true;
  yield();
  assert(0 && "should not reach here!");
}

// from, to 都是二级指针
void rt_hw_context_switch(rt_ubase_t from, rt_ubase_t to) { 
  from_sp_ptr = (Context **)from;
  to_sp_ptr = (Context **)to;
  need_sched = true;
  yield();
  assert(0 && "should not reach here!");
}

void rt_hw_context_switch_interrupt(void *context, rt_ubase_t from, rt_ubase_t to, struct rt_thread *to_thread) { assert(0); }

// 线程入口包装器的参数结构
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

/**
 * This function will initialize thread stack
 *
 * @param tentry the entry of thread
 * @param parameter the parameter of entry
 * @param stack_addr the beginning stack address
 * @param texit the function will be called when thread exit
 *
 * @return stack address
 */
rt_uint8_t *rt_hw_stack_init(void *tentry, void *parameter, rt_uint8_t *stack_addr, void *texit) {
  /// @ref
  /// https://www.rt-thread.org/document/site/#/rt-thread-version/rt-thread-standard/application-note/porting/risc-v/port-risc-v?id=risc-v-内核移植
  rt_uint8_t *stk = (rt_uint8_t *)RT_ALIGN_DOWN((rt_ubase_t)stack_addr, sizeof(rt_ubase_t));
  
  // 在栈上分配 thread_info_t 结构
  stk -= sizeof(thread_info_t);
  stk = (rt_uint8_t *)RT_ALIGN_DOWN((rt_ubase_t)stk, sizeof(rt_ubase_t));
  thread_info_t *info = (thread_info_t *)stk;
  info->tentry = tentry;
  info->texit = texit;
  info->parameter = parameter;
  
  Context *ctx = kcontext((Area){ NULL, stk }, entry_wrapper, info);
  return (void *)ctx;
}