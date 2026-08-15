#ifndef LETTUCE_TIDY_HEADER_INTERNAL_OBJECT_CHECK_H
#define LETTUCE_TIDY_HEADER_INTERNAL_OBJECT_CHECK_H

#include "clang-tidy/ClangTidyCheck.h"

namespace clang::tidy::lettuce {

// Warns about namespace-scope const definitions in project headers that have
// internal linkage. Each translation unit that includes such a header may emit
// a distinct object.
class HeaderInternalObjectCheck : public ClangTidyCheck {
public:
  HeaderInternalObjectCheck(llvm::StringRef Name, ClangTidyContext *Context)
      : ClangTidyCheck(Name, Context) {}

  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;
};

} // namespace clang::tidy::lettuce

#endif

