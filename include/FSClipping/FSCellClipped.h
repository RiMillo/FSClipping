#ifndef FSCELLCLIPPED_H
#define FSCELLCLIPPED_H

#include "FSClipping/FSBoundaryFaceProvider.h"
#include "FSClipping/FSFaceMatcher.h"
#include <span>

_FS_BEGIN_NAMESPACE

class FSCellClipped
{
public:
  FSCellClipped(const std::vector<FSFaceMatch>& matches, const BoundaryExtraction& be) : matches_(matches), be_(be)
  {
    FS_intT numCells = 0;
    for(const auto& t : be_.volumeCells) {
      numCells += t.second.size();
    }
    cellToMatches_.reserve(numCells);
    for(FS_intT m = 0; m < matches.size(); m++) {
      auto [it, inserted] = cellToMatches_.try_emplace(matches_[m].elemOwner1);
      cellToMatches_.at(matches_[m].elemOwner1).push_back(m);
    }
  };

  const std::span<const FS_intT> MatchesOf(FS_intT c);

  const FSFaceMatch& Match(FS_intT m) const noexcept { return matches_[m]; };
  bool Contains(FS_intT cell) const;
  bool IsReplacedSurface(FS_intT surfaceCell) const;
  // bool IsReplacedFace(FS_intT surfaceCell) const noexcept { return be_.surfaceCells.contains(surfaceCell); }

  template<class F> void ForEachMatch(F&& f) const
  {
    for(FS_intT m = 0; m < matches_.size(); m++) {
      f(m, matches_[m]);
    }
  }

private:
  const std::vector<FSFaceMatch>& matches_;
  const BoundaryExtraction& be_;

  std::unordered_map<FS_intT, std::vector<FS_intT> > cellToMatches_;
};

_FS_END_NAMESPACE

#endif