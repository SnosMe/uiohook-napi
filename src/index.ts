import { EventEmitter } from 'events'
import { join } from 'path'
const lib: AddonExports = require('node-gyp-build')(join(__dirname, '..'))

interface AddonExports {
  start(cb: (e: any) => void): void
  stop(): void
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
  Backspace: 0x000E,
  Tab: 0x000F,
  Enter: 0x001C,
  CapsLock: 0x003A,
  Escape: 0x0001,
  Space: 0x0039,
  PageUp: 0x0E49,
  PageDown: 0x0E51,
  End: 0x0E4F,
  Home: 0x0E47,
  ArrowLeft: 0xE04B,
  ArrowUp: 0xE048,
  ArrowRight: 0xE04D,
  ArrowDown: 0xE050,
  Insert: 0x0E52,
  Delete: 0x0E53,
  0: 0x000B,
  1: 0x0002,
  2: 0x0003,
  3: 0x0004,
  4: 0x0005,
  5: 0x0006,
  6: 0x0007,
  7: 0x0008,
  8: 0x0009,
  9: 0x000A,
  A: 0x001E,
  B: 0x0030,
  C: 0x002E,
  D: 0x0020,
  E: 0x0012,
  F: 0x0021,
  G: 0x0022,
  H: 0x0023,
  I: 0x0017,
  J: 0x0024,
  K: 0x0025,
  L: 0x0026,
  M: 0x0032,
  N: 0x0031,
  O: 0x0018,
  P: 0x0019,
  Q: 0x0010,
  R: 0x0013,
  S: 0x001F,
  T: 0x0014,
  U: 0x0016,
  V: 0x002F,
  W: 0x0011,
  X: 0x002D,
  Y: 0x0015,
  Z: 0x002C,
  Numpad0: 0x0052,
  Numpad1: 0x004F,
  Numpad2: 0x0050,
  Numpad3: 0x0051,
  Numpad4: 0x004B,
  Numpad5: 0x004C,
  Numpad6: 0x004D,
  Numpad7: 0x0047,
  Numpad8: 0x0048,
  Numpad9: 0x0049,
  NumpadMultiply: 0x0037,
  NumpadAdd: 0x004E,
  NumpadSubtract: 0x004A,
  NumpadDecimal: 0x0053,
  NumpadDivide: 0x0E35,
  F1: 0x003B,
  F2: 0x003C,
  F3: 0x003D,
  F4: 0x003E,
  F5: 0x003F,
  F6: 0x0040,
  F7: 0x0041,
  F8: 0x0042,
  F9: 0x0043,
  F10: 0x0044,
  F11: 0x0057,
  F12: 0x0058,
  F13: 0x005B,
  F14: 0x005C,
  F15: 0x005D,
  F16: 0x0063,
  F17: 0x0064,
  F18: 0x0065,
  F19: 0x0066,
  F20: 0x0067,
  F21: 0x0068,
  F22: 0x0069,
  F23: 0x006A,
  F24: 0x006B,
  Semicolon: 0x0027,
  Equal: 0x000D,
  Comma: 0x0033,
  Minus: 0x000C,
  Period: 0x0034,
  Slash: 0x0035,
  Backquote: 0x0029,
  BracketLeft: 0x001A,
  Backslash: 0x002B,
  BracketRight: 0x001B,
  Quote: 0x0028,
  Ctrl: 0x001D, // Left
  CtrlRight: 0x0E1D,
  Alt: 0x0038, // Left
  AltRight: 0x0E38,
  Shift: 0x002A, // Left
  ShiftRight: 0x0036,
  Meta: 0x0E5B,
  MetaRight: 0x0E5C
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
}

export const uIOhook = new UiohookNapi()
