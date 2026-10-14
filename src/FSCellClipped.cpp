#include "FSClipping/FSCellClipped.h"
#include <algorithm>

_FS_BEGIN_NAMESPACE

FSCellClipped::FSCellClipped(const std::vector<FSFaceMatch>& matches, const BoundaryExtraction& be)
  : matches_(matches), be_(be)
{
  cellToMatches_.reserve(be_.replacedFaces.size());

  for(FS_intT m = 0; m < FS_intT(matches_.size()); m++) {
    const FSFaceMatch& match = matches_[m];

    if(match.type == FSFaceMatch::UNKNOWN)
      FSError.SetAndPrintAndExit("FSCellClipped: a match has the UNKNOWN type.");
    if(match.clippedPoly3D.size() < 3)
      FSError.SetAndPrintAndExit("FSCellClipped: a match polygon has less than 3 vertices.");
    if(match.nodeGlobalIds.size() != match.clippedPoly3D.size())
      FSError.SetAndPrintAndExit("FSCellClipped: nodeGlobalIds and clippedPoly3D have different sizes.");

    cellToMatches_[match.elemOwner1].push_back(m);
  }

  // Every boundary cell is replaced by at least one polygon
  for(const auto& [cell, faces] : be_.replacedFaces) {
    if(!cellToMatches_.contains(cell))
      FSError.SetAndPrintAndExit("FSCellClipped: a boundary cell has no match.");
  }
}

bool FSCellClipped::IsReplacedSurface(FS_intT surfaceCell) const
{
  for(const auto& t : be_.surfaceCells)
    if(t.second.contains(surfaceCell))
      return true;
  return false;
}

bool FSCellClipped::IsReplacedFace(FS_intT cell, FS_intT localFace) const
{
  const auto it = be_.replacedFaces.find(cell);
  if(it == be_.replacedFaces.end())
    return false;
  return std::find(it->second.begin(), it->second.end(), localFace) != it->second.end();
}

std::span<const FS_intT> FSCellClipped::MatchesOf(FS_intT c) const
{
  auto it = cellToMatches_.find(c);
  if(it == cellToMatches_.end())
    FSError.SetAndPrintAndExit("FSCellClipped: Cell does not exist");
  return it->second;
}

_FS_END_NAMESPACE
