#include <stddef.h>
#include <stdint.h>

void* operator new(size_t, void* address) { return address; }
void operator delete(void*, void*) {}
extern "C" void __cxa_pure_virtual() {}

const uint16_t kCleanupValue = 0x5aa5;
volatile uint16_t cleanup_observed;

struct Base {
  virtual void touch() = 0;
  virtual void destroy() = 0;

 protected:
  ~Base() {}
};

struct Derived : Base {
  __attribute__((noinline)) ~Derived();
  void touch() { cleanup_observed = 0; }
  void destroy() { this->~Derived(); }
};

Derived::~Derived() { cleanup_observed = kCleanupValue; }

union Storage {
  uint8_t bytes[sizeof(Derived)];
  void* alignment;
};

__attribute__((noinline)) void exercise(Base* object) {
  object->touch();
  object->destroy();
}

int main() {
  Storage storage;
  Base* object = new (storage.bytes) Derived;
  exercise(object);
  return cleanup_observed == kCleanupValue ? 0 : 1;
}
