// Explicit instantiation of the hot PdmField<T> specializations. The matching
// `extern template class PdmField<T>;` declarations live in cafPdmField.h, suppressing
// re-instantiation in every TU that includes the header.

#include "cafPdmField.h"

// PdmFieldHandle::capability<T>() uses dynamic_cast on PdmFieldCapability*, which requires a
// complete polymorphic type at the point of instantiation. Bring it in here so the explicit
// instantiations of setValueWithFieldChanged() can compile.
#include "cafPdmFieldCapability.h"

namespace caf
{
template class PdmField<int>;
template class PdmField<unsigned int>;
template class PdmField<double>;
template class PdmField<float>;
template class PdmField<bool>;
template class PdmField<QString>;
template class PdmField<std::vector<int>>;
template class PdmField<std::vector<double>>;
template class PdmField<std::vector<QString>>;
template class PdmField<std::optional<double>>;
template class PdmField<std::optional<QString>>;
template class PdmField<std::pair<bool, double>>;
template class PdmField<std::pair<bool, QString>>;
} // namespace caf
