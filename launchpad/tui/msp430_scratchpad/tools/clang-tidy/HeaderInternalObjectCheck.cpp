#include "HeaderInternalObjectCheck.h"

#include "clang/AST/Decl.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Basic/Linkage.h"

using namespace clang::ast_matchers;

namespace clang::tidy::lettuce {

void HeaderInternalObjectCheck::registerMatchers(MatchFinder *Finder) {
  if (!getLangOpts().CPlusPlus)
    return;

  // Match definitions originating outside the current .cpp file. The callback
  // performs linkage and type checks that are clearer through Clang's semantic
  // API than through a large matcher expression.
  Finder->addMatcher(
      varDecl(isDefinition(), hasGlobalStorage(), unless(isImplicit()),
              unless(isExpansionInMainFile()),
              unless(isExpansionInSystemHeader()))
          .bind("variable"),
      this);
}

void HeaderInternalObjectCheck::check(
    const MatchFinder::MatchResult &Result) {
  const auto *Variable = Result.Nodes.getNodeAs<VarDecl>("variable");
  if (Variable == nullptr)
    return;

  // isFileVarDecl excludes local statics and static class data members. In
  // C++17 and newer, an inline variable is specifically intended for a header.
  if (!Variable->isFileVarDecl() || Variable->isInline() ||
      Variable->getFormalLinkage() != Linkage::Internal ||
      !Variable->getType().isConstQualified())
    return;

  // Macro-generated declarations are difficult to repair at the expansion
  // site and tend to be intentional implementation details.
  if (Variable->getLocation().isMacroID())
    return;

  if (Variable->hasConstantInitialization()) {
    diag(Variable->getLocation(),
         "internal-linkage const object %0 is defined in a header; each "
         "translation unit may emit a separate object if storage is required")
        << Variable;
  } else {
    diag(Variable->getLocation(),
         "internal-linkage const object %0 is defined in a header and requires "
         "dynamic initialization; each translation unit may emit a separate "
         "object")
        << Variable;
  }
}

} // namespace clang::tidy::lettuce

