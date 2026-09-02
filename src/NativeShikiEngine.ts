import type { TurboModule } from 'react-native'
import { TurboModuleRegistry } from 'react-native'

export interface Spec extends TurboModule {
  // oxlint-disable-next-line typescript/no-empty-object-type RN Codegen requires `{}` here
  readonly getConstants: () => {}
  readonly createScanner: (patterns: readonly string[], maxCacheSize: number) => number
  readonly findNextMatchSync: (
    scannerId: number,
    text: string,
    startPosition: number,
  ) => {
    readonly index: number
    readonly captureIndices: ReadonlyArray<{
      readonly start: number
      readonly end: number
      readonly length: number
    }>
  } | null
  readonly destroyScanner: (scannerId: number) => void
  readonly configureCache: (maxEntries: number, maxMemoryBytes: number) => void
  readonly clearPatternCache: () => void
  readonly trimMemory: () => void
  readonly getCacheStats: () => {
    readonly entryCount: number
    readonly estimatedBytes: number
    readonly scannerCount: number
    readonly maxEntries: number
    readonly maxBytes: number
  }
}

export default TurboModuleRegistry.getEnforcing<Spec>('ShikiEngine')
