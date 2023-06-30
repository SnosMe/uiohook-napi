import { EventEmitter } from 'events'
import { join } from 'path'
const lib: AddonExports = require('node-gyp-build')(join(__dirname, '..'))

interface AddonExports {
  start (cb: (e: any) => void): void
  stop (): void
  keyTap (key: number, type: KeyToggle): void
}

enum KeyToggle {
  Tap = 0,
  Down = 1,
  Up = 2
}

export enum EventType {
  EVENT_KEY_PRESSED = 4,
  EVENT_KEY_RELEASED = 5,
  EVENT_MOUSE_CLICKED = 6,
  EVENT_MOUSE_PRESSED = 7,
  EVENT_MOUSE_RELEASED = 8,
  EVENT_MOUSE_MOVED = 9,
  EVENT_MOUSE_WHEEL = 11
}

export interface UiohookKeyboardEvent {
  type: EventType.EVENT_KEY_PRESSED | EventType.EVENT_KEY_RELEASED
  time: number
  altKey: boolean
  ctrlKey: boolean
  metaKey: boolean
  shiftKey: boolean
  keycode: number
}

export interface UiohookMouseEvent {
  type: EventType.EVENT_MOUSE_CLICKED |
    EventType.EVENT_MOUSE_MOVED |
    EventType.EVENT_MOUSE_PRESSED |
    EventType.EVENT_MOUSE_RELEASED
  time: number
  altKey: boolean
  ctrlKey: boolean
  metaKey: boolean
  shiftKey: boolean
  x: number
  y: number
  button: unknown
  clicks: number
}

export interface UiohookWheelEvent {
  type: EventType.EVENT_MOUSE_WHEEL
  time: number
  altKey: boolean
  ctrlKey: boolean
  metaKey: boolean
  shiftKey: boolean
  x: number
  y: number
  clicks: number
  amount: number
  direction: WheelDirection
  rotation: number
}

export enum WheelDirection {
  VERTICAL = 3,
  HORIZONTAL = 4
}

export const UiohookKey = {
  Backspace: 0x0008,
  Tab: 0x0009,
  Enter: 0x000A,
  CapsLock: 0x0014,
  Escape: 0x001B,
  Space: 0x0020,
  PageUp: 0x0021,
  PageDown: 0x0022,
  End: 0x0023,
  Home: 0x0024,
  ArrowLeft: 0x0025,
  ArrowUp: 0x0026,
  ArrowRight: 0x0027,
  ArrowDown: 0x0028,
  Insert: 0x009B,
  Delete: 0x007F,
  0: 0x0030,
  1: 0x0031,
  2: 0x0032,
  3: 0x0033,
  4: 0x0034,
  5: 0x0035,
  6: 0x0036,
  7: 0x0037,
  8: 0x0038,
  9: 0x0039,
  A: 0x0041,
  B: 0x0042,
  C: 0x0043,
  D: 0x0044,
  E: 0x0045,
  F: 0x0046,
  G: 0x0047,
  H: 0x0048,
  I: 0x0049,
  J: 0x004A,
  K: 0x004B,
  L: 0x004C,
  M: 0x004D,
  N: 0x004E,
  O: 0x004F,
  P: 0x0050,
  Q: 0x0051,
  R: 0x0052,
  S: 0x0053,
  T: 0x0054,
  U: 0x0055,
  V: 0x0056,
  W: 0x0057,
  X: 0x0058,
  Y: 0x0059,
  Z: 0x005A,
  Numpad0: 0x0060,
  Numpad1: 0x0061,
  Numpad2: 0x0062,
  Numpad3: 0x0063,
  Numpad4: 0x0064,
  Numpad5: 0x0065,
  Numpad6: 0x0066,
  Numpad7: 0x0067,
  Numpad8: 0x0068,
  Numpad9: 0x0069,
  NumpadMultiply: 0x006A,
  NumpadAdd: 0x006B,
  NumpadSubtract: 0x006D,
  NumpadDecimal: 0x006C,
  NumpadDivide: 0x006F,
  NumpadEnd: 0xEE00 | 0x0061,
  NumpadArrowDown: 0xEE00 | 0x0062,
  NumpadPageDown: 0xEE00 | 0x0063,
  NumpadArrowLeft: 0xEE00 | 0x0064,
  NumpadArrowRight: 0xEE00 | 0x0066,
  NumpadHome: 0xEE00 | 0x0067,
  NumpadArrowUp: 0xEE00 | 0x0068,
  NumpadPageUp: 0xEE00 | 0x0069,
  NumpadInsert: 0xEE00 | 0x0060,
  NumpadDelete: 0xEE00 | 0x006C,
  F1: 0x0070,
  F2: 0x0071,
  F3: 0x0072,
  F4: 0x0073,
  F5: 0x0074,
  F6: 0x0075,
  F7: 0x0076,
  F8: 0x0077,
  F9: 0x0078,
  F10: 0x0079,
  F11: 0x007A,
  F12: 0x007B,
  F13: 0xF000,
  F14: 0xF001,
  F15: 0xF002,
  F16: 0xF003,
  F17: 0xF004,
  F18: 0xF005,
  F19: 0xF006,
  F20: 0xF007,
  F21: 0xF008,
  F22: 0xF009,
  F23: 0xF00A,
  F24: 0xF00B,
  Semicolon: 0x003B,
  Equal: 0x003D,
  Comma: 0x002C,
  Minus: 0x002D,
  Period: 0x002E,
  Slash: 0x002F,
  Backquote: 0x00C0,
  BracketLeft: 0x005B,
  Backslash: 0x005D,
  BracketRight: 0x005C,
  Quote: 0x00DE,
  Ctrl: 0xA011, // Left
  CtrlRight: 0xB011,
  Alt: 0xA012, // Left
  AltRight: 0xB012,
  Shift: 0xA010, // Left
  ShiftRight: 0xB010,
  Meta: 0xA09D,
  MetaRight: 0xB09D,
  NumLock: 0x0090,
  ScrollLock: 0x0091,
  PrintScreen: 0x009A,
} as const

declare interface UiohookNapi {
  on(event: 'input', listener: (e: UiohookKeyboardEvent | UiohookMouseEvent | UiohookWheelEvent) => void): this

  on(event: 'keydown', listener: (e: UiohookKeyboardEvent) => void): this
  on(event: 'keyup', listener: (e: UiohookKeyboardEvent) => void): this

  on(event: 'mousedown', listener: (e: UiohookMouseEvent) => void): this
  on(event: 'mouseup', listener: (e: UiohookMouseEvent) => void): this
  on(event: 'mousemove', listener: (e: UiohookMouseEvent) => void): this
  on(event: 'click', listener: (e: UiohookMouseEvent) => void): this

  on(event: 'wheel', listener: (e: UiohookWheelEvent) => void): this
}

class UiohookNapi extends EventEmitter {
  private handler (e: UiohookKeyboardEvent | UiohookMouseEvent | UiohookWheelEvent) {
    this.emit('input', e)
    switch (e.type) {
      case EventType.EVENT_KEY_PRESSED:
        this.emit('keydown', e)
        break
      case EventType.EVENT_KEY_RELEASED:
        this.emit('keyup', e)
        break
      case EventType.EVENT_MOUSE_CLICKED:
        this.emit('click', e)
        break
      case EventType.EVENT_MOUSE_MOVED:
        this.emit('mousemove', e)
        break
      case EventType.EVENT_MOUSE_PRESSED:
        this.emit('mousedown', e)
        break
      case EventType.EVENT_MOUSE_RELEASED:
        this.emit('mouseup', e)
        break
      case EventType.EVENT_MOUSE_WHEEL:
        this.emit('wheel', e)
        break
    }
  }

  start () {
    lib.start(this.handler.bind(this))
  }

  stop () {
    lib.stop()
  }

  keyTap (key: number, modifiers: number[] = []) {
    if (!modifiers.length) {
      lib.keyTap(key, KeyToggle.Tap)
      return
    }

    for (const modKey of modifiers) {
      lib.keyTap(modKey, KeyToggle.Down)
    }
    lib.keyTap(key, KeyToggle.Tap)
    let i = modifiers.length
    while (i--) {
      lib.keyTap(modifiers[i], KeyToggle.Up)
    }
  }

  keyToggle (key: number, toggle: 'down' | 'up') {
    lib.keyTap(key, (toggle === 'down' ? KeyToggle.Down : KeyToggle.Up))
  }
}

export const uIOhook = new UiohookNapi()
