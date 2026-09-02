#include "NativeShikiEngineModule.h"

#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace facebook::react {

namespace {

  std::mutex g_scanners_mutex;
  std::unordered_map<double, OnigContext*> g_scanners;
  double g_next_scanner_id = 1;

  thread_local std::string g_last_utf8;
  thread_local std::vector<int> g_last_b2u;

  bool is_ascii_only(const std::string& utf8) {
    for (unsigned char c : utf8) {
      if (c >= 0x80) {
        return false;
      }
    }
    return true;
  }

  std::vector<int> build_byte_to_utf16_table(const std::string& utf8) {
    std::vector<int> table(utf8.size() + 1);
    int u16 = 0;
    size_t i = 0;
    const size_t n = utf8.size();
    while (i < n) {
      const unsigned char c = static_cast<unsigned char>(utf8[i]);
      size_t len = 1;
      int units = 1;
      if (c < 0x80) {
        len = 1;
      } else if ((c & 0xE0) == 0xC0) {
        len = 2;
      } else if ((c & 0xF0) == 0xE0) {
        len = 3;
      } else if ((c & 0xF8) == 0xF0) {
        len = 4;
        units = 2;
      }
      for (size_t k = 0; k < len && i + k < n; k++) {
        table[i + k] = u16;
      }
      u16 += units;
      i += len;
    }
    table[n] = u16;
    return table;
  }

  int utf16_to_byte_offset(const std::vector<int>& table, int utf16_offset) {
    if (utf16_offset <= 0) {
      return 0;
    }
    const int n = static_cast<int>(table.size()) - 1;
    int lo = 0;
    int hi = n;
    while (lo < hi) {
      const int mid = lo + (hi - lo) / 2;
      if (table[mid] < utf16_offset) {
        lo = mid + 1;
      } else {
        hi = mid;
      }
    }
    return lo;
  }

  int byte_to_utf16_offset(const std::vector<int>& table, int byte_offset) {
    if (byte_offset < 0) {
      return byte_offset;
    }
    const int n = static_cast<int>(table.size()) - 1;
    if (byte_offset > n) {
      return table[n];
    }
    return table[byte_offset];
  }

}  // namespace

NativeShikiEngineModule::NativeShikiEngineModule(std::shared_ptr<CallInvoker> jsInvoker)
  : NativeShikiEngineCxxSpec<NativeShikiEngineModule>(std::move(jsInvoker)) {}

NativeShikiEngineModule::~NativeShikiEngineModule() {
  std::lock_guard<std::mutex> lock(g_scanners_mutex);
  for (const double scanner_id : owned_scanner_ids_) {
    auto it = g_scanners.find(scanner_id);
    if (it == g_scanners.end()) {
      continue;
    }
    free_scanner(it->second);
    g_scanners.erase(it);
  }
  owned_scanner_ids_.clear();
}

jsi::Object NativeShikiEngineModule::getConstants(jsi::Runtime& rt) {
  return jsi::Object(rt);
}

double NativeShikiEngineModule::createScanner(jsi::Runtime& rt, jsi::Array patterns, double maxCacheSize) {
  const size_t pattern_count = patterns.length(rt);
  std::vector<std::string> pattern_strings;
  std::vector<const char*> pattern_ptrs;
  pattern_strings.reserve(pattern_count);
  pattern_ptrs.reserve(pattern_count);

  for (size_t i = 0; i < pattern_count; i++) {
    jsi::String pattern = patterns.getValueAtIndex(rt, i).asString(rt);
    pattern_strings.push_back(pattern.utf8(rt));
    pattern_ptrs.push_back(pattern_strings.back().c_str());
  }

  const size_t cache_hint = maxCacheSize > 0 ? static_cast<size_t>(maxCacheSize) : 0;
  OnigContext* context = create_scanner(pattern_ptrs.data(), static_cast<int>(pattern_count), cache_hint);

  if (!context) {
    throw jsi::JSError(rt, "Failed to create scanner");
  }

  std::lock_guard<std::mutex> lock(g_scanners_mutex);
  const double scanner_id = g_next_scanner_id++;
  g_scanners[scanner_id] = context;
  owned_scanner_ids_.insert(scanner_id);
  return scanner_id;
}

std::optional<jsi::Object>
NativeShikiEngineModule::findNextMatchSync(jsi::Runtime& rt, double scannerId, jsi::String text, double startPosition) {
  OnigContext* context = nullptr;
  {
    std::lock_guard<std::mutex> lock(g_scanners_mutex);
    auto it = g_scanners.find(scannerId);
    if (it == g_scanners.end()) {
      throw jsi::JSError(rt, "Invalid scanner ID");
    }
    context = it->second;
  }

  std::string text_str = text.utf8(rt);

  int start_byte = 0;
  const bool ascii = is_ascii_only(text_str);

  if (ascii) {
    g_last_utf8.clear();
    g_last_b2u.clear();
    start_byte = static_cast<int>(startPosition);
    if (start_byte < 0) {
      start_byte = 0;
    }
  } else {
    if (text_str != g_last_utf8) {
      g_last_utf8 = text_str;
      g_last_b2u = build_byte_to_utf16_table(text_str);
    }
    start_byte = utf16_to_byte_offset(g_last_b2u, static_cast<int>(startPosition));
  }

  OnigResult* result = find_next_match(context, text_str.c_str(), start_byte);
  if (!result) {
    return std::nullopt;
  }

  jsi::Object match_obj(rt);
  match_obj.setProperty(rt, "index", result->pattern_index);

  jsi::Array capture_indices(rt, result->capture_count);
  for (int i = 0; i < result->capture_count; i++) {
    jsi::Object capture(rt);
    int start = result->capture_indices[i * 2];
    int end = result->capture_indices[i * 2 + 1];

    if (!ascii) {
      if (start >= 0) {
        start = byte_to_utf16_offset(g_last_b2u, start);
      }
      if (end >= 0) {
        end = byte_to_utf16_offset(g_last_b2u, end);
      }
    }

    capture.setProperty(rt, "start", start);
    capture.setProperty(rt, "end", end);
    capture.setProperty(rt, "length", end - start);
    capture_indices.setValueAtIndex(rt, i, std::move(capture));
  }
  match_obj.setProperty(rt, "captureIndices", std::move(capture_indices));

  free_result(result);
  return match_obj;
}

void NativeShikiEngineModule::destroyScanner(jsi::Runtime& rt, double scannerId) {
  OnigContext* context = nullptr;
  {
    std::lock_guard<std::mutex> lock(g_scanners_mutex);
    auto it = g_scanners.find(scannerId);
    if (it == g_scanners.end()) {
      return;
    }
    context = it->second;
    g_scanners.erase(it);
    owned_scanner_ids_.erase(scannerId);
  }
  free_scanner(context);
}

void NativeShikiEngineModule::configureCache(jsi::Runtime& rt, double maxEntries, double maxMemoryBytes) {
  const size_t entries = maxEntries > 0 ? static_cast<size_t>(maxEntries) : 0;
  const size_t bytes = maxMemoryBytes > 0 ? static_cast<size_t>(maxMemoryBytes) : 0;
  configure_pattern_cache(entries, bytes);
}

void NativeShikiEngineModule::clearPatternCache(jsi::Runtime& rt) {
  clear_unused_pattern_cache();
  g_last_utf8.clear();
  g_last_b2u.clear();
}

void NativeShikiEngineModule::trimMemory(jsi::Runtime& rt) {
  trim_pattern_cache();
  g_last_utf8.clear();
  g_last_b2u.clear();
}

jsi::Object NativeShikiEngineModule::getCacheStats(jsi::Runtime& rt) {
  const OnigCacheStats stats = get_pattern_cache_stats();
  jsi::Object obj(rt);
  obj.setProperty(rt, "entryCount", static_cast<double>(stats.entry_count));
  obj.setProperty(rt, "estimatedBytes", static_cast<double>(stats.estimated_bytes));
  obj.setProperty(rt, "scannerCount", static_cast<double>(stats.scanner_count));
  obj.setProperty(rt, "maxEntries", static_cast<double>(stats.max_entries));
  obj.setProperty(rt, "maxBytes", static_cast<double>(stats.max_bytes));
  return obj;
}

}  // namespace facebook::react
