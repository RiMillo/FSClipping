#include "FSClipping/FSNodeRegistry.h"
#include <FSMeshData.h>

_FS_BEGIN_NAMESPACE

FSNodeRegistry::FSNodeRegistry(FSMesh& mesh, const std::vector<FSCellSource>& sources, const FSCellClipped& matches,
                               FS_floatT tol)
{
  FSFloatArrayT oldCoords;
  FS_intT nodeOffset;
  FSQuantityDescArrayT coordDesc;
  mesh.GetMeshData()->GetUnstructCells().GetCoordinates3D(coordDesc, oldCoords, nodeOffset);

  for(FS_intT n = 0; n < oldCoords.Size(); n++) {
    FS_floatT x = oldCoords(n, 0);
    FS_floatT y = oldCoords(n, 1);
    FS_floatT z = oldCoords(n, 2);
  }
}

_FS_END_NAMESPACE