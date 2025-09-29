#include <immintrin.h>  // For _mm_pause() on x86/x64

#include <atomic>

class SpinLock {
  bool m_bypass;
  std::atomic_flag m_flag;

 public:
  SpinLock(bool bypass) : m_bypass{bypass}, m_flag{ATOMIC_FLAG_INIT} {}

  void lock() {
    if (m_bypass) {
      return;
    }

    while (m_flag.test_and_set(std::memory_order_acquire)) {
      _mm_pause();
    }
  }

  void unlock() {
    if (m_bypass) {
      return;
    }
    m_flag.clear(std::memory_order_release);
  }
};

class SpinLockGaurd {
  SpinLock& m_spinLock;

 public:
  SpinLockGaurd(SpinLock& spinLock) : m_spinLock{spinLock} {
    m_spinLock.lock();
  }
  ~SpinLockGaurd() { m_spinLock.unlock(); }
};