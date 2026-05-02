#pragma once

#define CAF_IS_DEFINING_PDM_FIELD
#define PdmDataValueField PdmField
#include "cafPdmDataValueField.h"
#undef PdmDataValueField
#undef CAF_IS_DEFINING_PDM_FIELD

// Suppress per-TU instantiation of the hot PdmField<T> specializations used throughout the
// codebase. The matching `template class PdmField<T>;` lines live in cafPdmField.cpp.
//
// extern template only suppresses out-of-class members and the vtable; in-class inline members
// (value(), operator==, etc.) are unaffected and still inlined where called.
//
// The includes for QString / std::vector / std::optional / std::pair come transitively via
// cafPdmDataValueField.h (QVariant, <vector>, <optional>, etc.). cafFilePath.h is the only
// header we need to add explicitly.

#include "cafFilePath.h"

namespace caf
{
extern template class PdmField<int>;
extern template class PdmField<unsigned int>;
extern template class PdmField<double>;
extern template class PdmField<float>;
extern template class PdmField<bool>;
extern template class PdmField<QString>;
extern template class PdmField<caf::FilePath>;
extern template class PdmField<std::vector<int>>;
extern template class PdmField<std::vector<double>>;
extern template class PdmField<std::vector<QString>>;
extern template class PdmField<std::vector<caf::FilePath>>;
extern template class PdmField<std::optional<double>>;
extern template class PdmField<std::optional<QString>>;
extern template class PdmField<std::pair<bool, double>>;
extern template class PdmField<std::pair<bool, QString>>;
} // namespace caf
