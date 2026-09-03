#pragma once

#include <ReactCommon/CallInvoker.h>

#include <jsi/jsi.h>

#include <memory>
#include <optional>
#include <unordered_set>

#if __has_include("codegen/NativeShikiEngineSpec.g.h")
#  include "codegen/NativeShikiEngineDataTypes.g.h"
#  include "codegen/NativeShikiEngineSpec.g.h"
#else
#  error "Could not find NativeShikiEngineSpec.g.h - ensure codegen has run and New Architecture is enabled"
#endif

#if __has_include("onig_regex.h")
#  include "onig_regex.h"
#endif

namespace facebook::react {

class NativeShikiEngineModule {
 public:
  NativeShikiEngineModule(std::shared_ptr<CallInvoker> jsInvoker);
  ~NativeShikiEngineModule();

  jsi::Object getConstants(jsi::Runtime& rt);
  double createScanner(jsi::Runtime& rt, jsi::Array patterns, double maxCacheSize);
  std::optional<jsi::Object>
  findNextMatchSync(jsi::Runtime& rt, double scannerId, jsi::String text, double startPosition);
  void destroyScanner(jsi::Runtime& rt, double scannerId);
  void configureCache(jsi::Runtime& rt, double maxEntries, double maxMemoryBytes);
  void clearPatternCache(jsi::Runtime& rt);
  void trimMemory(jsi::Runtime& rt);
  jsi::Object getCacheStats(jsi::Runtime& rt);

 private:
  std::unordered_set<double> owned_scanner_ids_;
};

}  // namespace facebook::react
