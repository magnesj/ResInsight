#include "cafAppEnum.h"
#include "cafPdmFieldHandle.h"

namespace caf
{

QString appEnumCreateSubsetKey( PdmFieldHandle* fieldHandle )
{
    if ( !fieldHandle ) return QString();

    // Create a unique key by combining the owner class name with the field keyword.
    // This prevents collisions when different object types use the same field keyword.
    return fieldHandle->ownerClass() + "::" + fieldHandle->keyword();
}

} // namespace caf
