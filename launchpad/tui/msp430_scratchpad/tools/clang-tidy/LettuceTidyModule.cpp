#include "HeaderInternalObjectCheck.h"

#include "clang-tidy/ClangTidyModule.h"

namespace clang::tidy::lettuce {

class LettuceTidyModule : public ClangTidyModule {
public:
  void addCheckFactories(ClangTidyCheckFactories &Factories) override {
    Factories.registerCheck<HeaderInternalObjectCheck>(
        "lettuce-header-internal-object");
  }
};

static ClangTidyModuleRegistry::Add<LettuceTidyModule>
    Registration("lettuce-module", "Adds Lettuce project checks.");

} // namespace clang::tidy::lettuce

// Keep an anchor available for build systems that link this module statically.
volatile int LettuceTidyModuleAnchorSource = 0;
