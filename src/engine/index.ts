import type { PatternScanner, RegexEngine } from '@shikijs/types'
import type { FindOption, IOnigMatch, OnigString } from '@shikijs/vscode-textmate'
import type { Spec } from '../NativeShikiEngine'
import { TurboModuleRegistry } from 'react-native'
import { convertToOnigMatch } from './utils'

export interface NativeEngineOptions {
  maxCacheEntries?: number
  maxMemoryBytes?: number
}

export interface CacheStats {
  entryCount: number
  estimatedBytes: number
  scannerCount: number
  maxEntries: number
  maxBytes: number
}

export type NativeRegexEngine = RegexEngine & {
  getCacheStats: () => CacheStats
  clearPatternCache: () => void
  trimMemory: () => void
  dispose: () => void
}

function assertNonNegativeInt(value: number | undefined, name: string): number | undefined {
  if (value === undefined) return undefined
  if (!Number.isFinite(value) || value < 0 || !Number.isInteger(value))
    throw new TypeError(`${name} must be a non-negative integer`)
  return value
}

function getModule(): Spec | null {
  return TurboModuleRegistry.get<Spec>('ShikiEngine') ?? null
}

export function createNativeEngine(options: NativeEngineOptions = {}): NativeRegexEngine {
  const ShikiEngine = getModule()
  if (!ShikiEngine) throw new Error('Native engine not available')

  const maxCacheEntries = assertNonNegativeInt(options.maxCacheEntries, 'maxCacheEntries')
  const maxMemoryBytes = assertNonNegativeInt(options.maxMemoryBytes, 'maxMemoryBytes')

  ShikiEngine.configureCache(maxCacheEntries ?? 0, maxMemoryBytes ?? 0)

  const scannerIds = new Set<number>()

  const engine: NativeRegexEngine = {
    createScanner(patterns: (string | RegExp)[]): PatternScanner {
      if (
        !Array.isArray(patterns) ||
        patterns.some(p => typeof p !== 'string' && !(p instanceof RegExp))
      )
        throw new TypeError('Patterns must be an array of strings or RegExp objects')

      const stringPatterns = patterns.map(p => (typeof p === 'string' ? p : p.source))

      const scannerId = ShikiEngine.createScanner(stringPatterns, maxCacheEntries ?? 1000)
      if (typeof scannerId !== 'number') throw new TypeError('Failed to create native scanner')
      scannerIds.add(scannerId)

      return {
        findNextMatchSync(
          string: string | OnigString,
          startPosition: number,
          _options: FindOption,
        ): IOnigMatch | null {
          if (startPosition < 0) throw new RangeError('Start position must be >= 0')

          const stringContent = typeof string === 'string' ? string : string.content
          if (typeof stringContent !== 'string') throw new TypeError('Invalid input string')

          const result = ShikiEngine.findNextMatchSync(scannerId, stringContent, startPosition)
          return convertToOnigMatch(result)
        },

        dispose(): void {
          try {
            ShikiEngine.destroyScanner(scannerId)
          } finally {
            scannerIds.delete(scannerId)
          }
        },
      }
    },

    createString(s: string): OnigString {
      if (typeof s !== 'string') throw new TypeError('Input must be a string')
      return { content: s }
    },

    getCacheStats(): CacheStats {
      return ShikiEngine.getCacheStats()
    },

    clearPatternCache(): void {
      ShikiEngine.clearPatternCache()
    },

    trimMemory(): void {
      ShikiEngine.trimMemory()
    },

    dispose(): void {
      for (const id of scannerIds) {
        try {
          ShikiEngine.destroyScanner(id)
        } catch {
          // ignore, scanner may already be gone
        }
      }
      scannerIds.clear()
      ShikiEngine.clearPatternCache()
    },
  }

  return engine
}

export function isNativeEngineAvailable(): boolean {
  return getModule() != null
}
