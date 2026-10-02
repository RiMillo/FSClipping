#ifndef FSCELLCLIPPED_H
#define FSCELLCLIPPED_H

#include "FSClipping/FSBoundaryFaceProvider.h"
#include "FSClipping/FSFaceMatcher.h"
#include <span>

_FS_BEGIN_NAMESPACE

class FSCellClipped
{
public:
  FSCellClipped(const std::vector<FSFaceMatch>& matches, const BoundaryExtraction& be) : matches_(matches), be_(be) {};

  std::vector<FS_intT> MatchesOf(FS_intT c);

  const FSFaceMatch& Match(FS_intT m) const noexcept { return matches_[m]; };
  bool Contains(FS_intT cell) const noexcept { return be_.volumeCells.contains(cell); };

private:
  const std::vector<FSFaceMatch>& matches_;
  const BoundaryExtraction& be_;

  // std::unordered_map<std::pair<FS_intT, FS_intT>, std::span<FS_intT> > cellToMatches;
};

_FS_END_NAMESPACE

#endif