#include "FSClipping/FSCellClipped.h"

_FS_BEGIN_NAMESPACE

bool FSCellClipped::Contains(FS_intT cell) const
{
  for(const auto& [t, cells] : be_.volumeCells) {
    if(cells.contains(cell)) {
      return true;
    }
  }
  return false;
}

bool FSCellClipped::IsReplacedSurface(FS_intT surfaceCell) const
{
  for(const auto& t : be_.surfaceCells)
    if(t.second.contains(surfaceCell))
      return true;
  return false;
}

const std::span<const FS_intT> FSCellClipped::MatchesOf(FS_intT c)
{
  if(!cellToMatches_.contains(c))
    FSError.SetAndPrintAndExit("FSCellClipped: Cell does not exist");
  return cellToMatches_.at(c);
}

_FS_END_NAMESPACE