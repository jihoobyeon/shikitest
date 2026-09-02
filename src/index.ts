import type { CacheStats, NativeEngineOptions, NativeRegexEngine } from './engine'
import { createNativeEngine, isNativeEngineAvailable } from './engine'
import { attachMemoryPressureHandler } from './engine/memoryPressure'

export type { Spec } from './NativeShikiEngine'
export type { CacheStats, NativeEngineOptions, NativeRegexEngine }
export { attachMemoryPressureHandler, createNativeEngine, isNativeEngineAvailable }
