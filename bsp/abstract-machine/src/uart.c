/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 */

#include "amdev.h"
#include <am.h>
#include <klib-macros.h>
#include <klib.h>
#include <rtdevice.h>
#include <rtthread.h>

#define UART_DEFAULT_BAUDRATE 115200

struct device_uart {
  rt_ubase_t hw_base;
  rt_uint32_t irqno;
};

static void *uart0_base = (void *)0x10000000;
static struct rt_serial_device serial0;
static struct device_uart uart0;

static rt_err_t _uart_configure(struct rt_serial_device *serial,
                                struct serial_configure *cfg) {
  return (RT_EOK);
}

static rt_err_t _uart_control(struct rt_serial_device *serial, int cmd,
                              void *arg) {
  return (RT_EOK);
}

static int _uart_putc(struct rt_serial_device *serial, char c) {
  putch(c);
  return 1;
}

const static char lut[256] = {
    [AM_KEY_F1] = '\0',
    [AM_KEY_F2] = '\0',
    [AM_KEY_F3] = '\0',
    [AM_KEY_F4] = '\0',
    [AM_KEY_F5] = '\0',
    [AM_KEY_F6] = '\0',
    [AM_KEY_F7] = '\0',
    [AM_KEY_F8] = '\0',
    [AM_KEY_F9] = '\0',
    [AM_KEY_F10] = '\0',
    [AM_KEY_F11] = '\0',
    [AM_KEY_F12] = '\0',

    [AM_KEY_GRAVE] = '~',
    [AM_KEY_1] = '1',
    [AM_KEY_2] = '2',
    [AM_KEY_3] = '3',
    [AM_KEY_4] = '4',
    [AM_KEY_5] = '5',
    [AM_KEY_6] = '6',
    [AM_KEY_7] = '7',
    [AM_KEY_8] = '8',
    [AM_KEY_9] = '9',
    [AM_KEY_0] = '0',
    [AM_KEY_MINUS] = '_',
    [AM_KEY_EQUALS] = '=',
    [AM_KEY_BACKSPACE] = '\b',

    [AM_KEY_TAB] = '\t',
    [AM_KEY_Q] = 'q',
    [AM_KEY_W] = 'w',
    [AM_KEY_E] = 'e',
    [AM_KEY_R] = 'r',
    [AM_KEY_T] = 't',
    [AM_KEY_Y] = 'y',
    [AM_KEY_U] = 'u',
    [AM_KEY_I] = 'i',
    [AM_KEY_O] = 'o',
    [AM_KEY_P] = 'p',
    [AM_KEY_LEFTBRACKET] = '[',
    [AM_KEY_RIGHTBRACKET] = ']',
    [AM_KEY_BACKSLASH] = '\\',

    [AM_KEY_CAPSLOCK] = '\0',
    [AM_KEY_A] = 'a',
    [AM_KEY_S] = 's',
    [AM_KEY_D] = 'd',
    [AM_KEY_F] = 'f',
    [AM_KEY_G] = 'g',
    [AM_KEY_H] = 'h',
    [AM_KEY_J] = 'j',
    [AM_KEY_K] = 'k',
    [AM_KEY_L] = 'l',
    [AM_KEY_SEMICOLON] = ';',
    [AM_KEY_APOSTROPHE] = '\'',
    [AM_KEY_RETURN] = '\n',

    [AM_KEY_LSHIFT] = '\0',
    [AM_KEY_Z] = 'z',
    [AM_KEY_X] = 'x',
    [AM_KEY_C] = 'c',
    [AM_KEY_V] = 'v',
    [AM_KEY_B] = 'b',
    [AM_KEY_N] = 'n',
    [AM_KEY_M] = 'm',
    [AM_KEY_COMMA] = ',',
    [AM_KEY_PERIOD] = '.',
    [AM_KEY_SLASH] = '/',
    [AM_KEY_RSHIFT] = '\0',

    [AM_KEY_LCTRL] = '\0',
    [AM_KEY_APPLICATION] = '\0',
    [AM_KEY_LALT] = '\0',
    [AM_KEY_SPACE] = ' ',
    [AM_KEY_RALT] = '\0',
    [AM_KEY_RCTRL] = '\0',

    [AM_KEY_UP] = '\0',
    [AM_KEY_DOWN] = '\0',
    [AM_KEY_LEFT] = '\0',
    [AM_KEY_RIGHT] = '\0',
    [AM_KEY_INSERT] = '\0',
    [AM_KEY_DELETE] = '\0',
    [AM_KEY_HOME] = '\0',
    [AM_KEY_END] = '\0',
    [AM_KEY_PAGEUP] = '\0',
    [AM_KEY_PAGEDOWN] = '\0',
};

static bool shift_down = false;

__attribute__((unused))
static char non_func_key(int keycode) {
  if (shift_down) {
    if (keycode == AM_KEY_MINUS) {
      return '_';
    } else if (AM_KEY_A <= keycode && keycode <= AM_KEY_Z) {
      return lut[keycode] - 'a' + 'A';
    } else if (AM_KEY_1 == keycode) {
      return '!';
    } else if (AM_KEY_2 == keycode) {
      return '@';
    } else if (AM_KEY_3 == keycode) {
      return '#';
    } else if (AM_KEY_4 == keycode) {
      return '$';
    } else if (AM_KEY_5 == keycode) {
      return '%';
    } else if (AM_KEY_6 == keycode) {
      return '^';
    } else if (AM_KEY_7 == keycode) {
      return '&';
    } else if (AM_KEY_8 == keycode) {
      return '*';
    } else if (AM_KEY_9 == keycode) {
      return '(';
    } else if (AM_KEY_0 == keycode) {
      return ')';
    } else if (AM_KEY_MINUS == keycode) {
      return '_';
    } else if (AM_KEY_EQUALS == keycode) {
      return '+';
    } else if (AM_KEY_LEFTBRACKET == keycode) {
      return '{';
    } else if (AM_KEY_RIGHTBRACKET == keycode) {
      return '}';
    } else if (AM_KEY_BACKSLASH == keycode) {
      return '|';
    } else if (AM_KEY_SEMICOLON == keycode) {
      return ':';
    } else if (AM_KEY_APOSTROPHE == keycode) {
      return '"';
    } else if (AM_KEY_COMMA == keycode) {
      return '<';
    } else if (AM_KEY_PERIOD == keycode) {
      return '>';
    } else if (AM_KEY_SLASH == keycode) {
      return '?';
    } else {
      return lut[keycode];
    }
  } else {
    return lut[keycode];
  }
}

static int _uart_getc(struct rt_serial_device *serial) {
  AM_INPUT_KEYBRD_T ev = io_read(AM_INPUT_KEYBRD);
  if (ev.keycode) {
    if (ev.keydown) {
      if (lut[ev.keycode]) {
        return lut[ev.keycode];
      }
    } else { // release
    }
  }
  return -1;
}

const struct rt_uart_ops _uart_ops = {_uart_configure, _uart_control,
                                      _uart_putc, _uart_getc,
                                      // TODO: add DMA support
                                      RT_NULL};

/*
 * UART Initiation
 */
int rt_hw_uart_init(void) {
  struct rt_serial_device *serial;
  struct device_uart *uart;
  struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;
  // register device
  serial = &serial0;
  uart = &uart0;

  serial->ops = &_uart_ops;
  serial->config = config;
  serial->config.baud_rate = UART_DEFAULT_BAUDRATE;
  uart->hw_base = (rt_ubase_t)uart0_base;
  uart->irqno = 0x0a;

  rt_hw_serial_register(serial, RT_CONSOLE_DEVICE_NAME,
                        RT_DEVICE_FLAG_STREAM | RT_DEVICE_FLAG_RDWR |
                            RT_DEVICE_FLAG_INT_RX,
                        uart);
  return 0;
}

/* WEAK for SDK 0.5.6 */
rt_weak void uart_debug_init(int uart_channel) {}
