import type { EventSubscription } from 'react-native'
import type { NativeRegexEngine } from './index'
import { AppState, Platform } from 'react-native'

export function attachMemoryPressureHandler(engine: NativeRegexEngine): () => void {
  const subs: EventSubscription[] = []

  if (Platform.OS === 'ios') {
    subs.push(
      AppState.addEventListener('memoryWarning', () => {
        engine.trimMemory()
      }),
    )
  }

  subs.push(
    AppState.addEventListener('change', state => {
      if (state === 'background') engine.trimMemory()
    }),
  )

  return () => {
    for (const sub of subs) sub.remove()
  }
}
