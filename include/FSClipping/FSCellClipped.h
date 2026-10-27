#ifndef FSCELLCLIPPED_H
#define FSCELLCLIPPED_H

#include "FSClipping/FSBoundaryFaceProvider.h"
#include "FSClipping/FSFaceMatcher.h"
#include <span>
#include <unordered_map>
#include <vector>

_FS_BEGIN_NAMESPACE

// Only references to the matches and the boundary extraction are kept: both must outlive this object.
class FSCellClipped
{
public:
  FSCellClipped(const std::vector<FSFaceMatch>& matches, const BoundaryExtraction& be);

  std::span<const FS_intT> MatchesOf(FS_intT c) const;

  const FSFaceMatch& Match(FS_intT m) const noexcept { return matches_[m]; }
  bool HasBoundaryFace(FS_intT cell) const noexcept { return be_.cell2BoundaryFaces.contains(cell); }
  bool IsBoundarySurface(FS_intT surfaceCell) const;
  bool IsBoundaryFace(FS_intT cell, FS_intT localFace) const;

  template<class F> void ForEachMatch(F&& f) const
  {
    for(FS_intT m = 0; m < FS_intT(matches_.size()); m++) {
      f(m, matches_[m]);
    }
  }

private:
  const std::vector<FSFaceMatch>& matches_;
  const BoundaryExtraction& be_;

  std::unordered_map<FS_intT, std::vector<FS_intT> > cell2Matches_; // elemOwner1 -> match indices, increasing
};

_FS_END_NAMESPACE

#endif
