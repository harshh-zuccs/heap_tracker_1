// Copyright 2017, Arun Saha <arunksaha@gmail.com>

#include "heap_tracker_observer_timeseries_file.h"

#include <thread>
#include <atomic>
#include <chrono>

HeapObserverTimeseriesFile::HeapObserverTimeseriesFile(
  HeapTrackOptions const heap_track_options, char const * const filename)
    : heap_track_options_{heap_track_options},
      filename_{filename},
      out_{filename_, std::ofstream::out | std::ofstream::trunc},
      stop_thread_{false} {
  TRACE;
  assert(filename_);
  StartFlushThread();  // Start background flushing
}

HeapObserverTimeseriesFile::~HeapObserverTimeseriesFile() {
  TRACE;
  StopFlushThread();   // Stop background flushing thread
  Flush();             // Final flush before closing
  out_.close();

  char buffer[256] = {};
  snprintf(buffer, sizeof buffer, "Observer Timeseries saved to file %s\n", filename_);
  write(STDOUT_FILENO, buffer, sizeof buffer);
}

HeapTrackOptions
HeapObserverTimeseriesFile::GetHeapTrackOptions() const {
  return heap_track_options_;
}

void
HeapObserverTimeseriesFile::OnAlloc(AllocCallbackInfo const & alloc_cb_info) {
  TRACE;
  offline_entries_[size_++] = OfflineEntry{alloc_cb_info};
  FlushIfFull();
}

void
HeapObserverTimeseriesFile::OnFree(FreeCallbackInfo const & free_cb_info) {
  TRACE;
  offline_entries_[size_++] = OfflineEntry{free_cb_info};
  FlushIfFull();
}

void
HeapObserverTimeseriesFile::OnComplete() {
  TRACE;
  Flush();
}

void
HeapObserverTimeseriesFile::Dump() const {}

void
HeapObserverTimeseriesFile::Reset() {
  assert(0);
}

void
HeapObserverTimeseriesFile::Flush() {
  TRACE;
  for (size_t i = 0; i != size_; ++i) {
    out_ << offline_entries_[i];
  }
  size_ = 0;
  out_.flush();
}

void
HeapObserverTimeseriesFile::FlushIfFull() {
  if (size_ == kMaxBufferCount) {
    Flush();
  }
}

// ===== Periodic Flushing Thread =====

void HeapObserverTimeseriesFile::StartFlushThread() {
  flush_thread_ = std::thread([this]() {
    while (!stop_thread_) {
      std::this_thread::sleep_for(std::chrono::seconds(10));
      Flush();
    }
  });
}

void HeapObserverTimeseriesFile::StopFlushThread() {
  stop_thread_ = true;
  if (flush_thread_.joinable()) {
    flush_thread_.join();
  }
}

#if 0
void
HeapObserverTimeseriesFile::OpenIfNot() {
  if (!out_.is_open()) {
    out_.open(filename_, std::ofstream::out | std::ofstream::trunc);
    if (!out_.is_open()) {
      std::cerr << "Error: Could not open file = " << filename_
        << "; error = " << strerror(errno) << '\n';
      exit(1);
    }
  }
}
#endif
