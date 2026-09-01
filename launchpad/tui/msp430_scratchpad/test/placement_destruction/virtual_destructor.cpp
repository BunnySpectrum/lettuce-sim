#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

void* operator new(size_t, void* address) { return address; }
__attribute__((noinline)) void operator delete(void* address) { free(address); }
void operator delete(void*, void*) {}
extern "C" void __cxa_pure_virtual() {}

const uint16_t kCleanupValue = 0x5aa5;
volatile uint16_t cleanup_observed;

struct Base {
  virtual ~Base() {}
  virtual void touch() = 0;
};

struct Derived : Base {
  __attribute__((noinline)) ~Derived();
  void touch() { cleanup_observed = 0; }
};

Derived::~Derived() { cleanup_observed = kCleanupValue; }

union Storage {
  uint8_t bytes[sizeof(Derived)];
  void* alignment;
};

__attribute__((noinline)) void exercise(Base* object) {
  object->touch();
  object->~Base();
}

int main() {
  Storage storage;
  Base* object = new (storage.bytes) Derived;
  exercise(object);
  return cleanup_observed == kCleanupValue ? 0 : 1;
}
