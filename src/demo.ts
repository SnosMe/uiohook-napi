import { uIOhook, UiohookKey } from './'

const keycodeMap = new Map(Object.entries(UiohookKey).map(_ => [_[1], _[0]]))

;(function main () {
  uIOhook.on('keydown', (e) => {
    console.log(
      `${prettyModifier('ctrl', e.ctrlKey)}${prettyModifier('shift', e.shiftKey)}${prettyModifier('alt', e.altKey)}`,
      e.keycode,
      keycodeMap.get(e.keycode as any)
    )

    if (e.keycode === UiohookKey.Escape) {
      process.exit(0)
    }
  })
  
  uIOhook.start()
})()

function prettyModifier (name: string, state: boolean) {
  return state ? `[${name}]` : ` ${name} `
}
