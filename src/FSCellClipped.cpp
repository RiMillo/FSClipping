#include "FSClipping/FSCellClipped.h"

_FS_BEGIN_NAMESPACE

std::vector<FS_intT> FSCellClipped::MatchesOf(FS_intT c)
{
  std::vector<FS_intT> matchesIndex;
  for(FS_intT m = 0; m < matches_.size(); m++) {
    if(matches_[m].elemOwner1 == c)
      matchesIndex.push_back(m);
  }
  return matchesIndex;
}

_FS_END_NAMESPACE