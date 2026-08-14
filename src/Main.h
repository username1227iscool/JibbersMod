#pragma once

// The per-variable display names, field names, and slider limits that used
// to live here (SliderName1/2, FieldName1/2, LowerLimit1/2, UperLimit1/2)
// have all moved into the single g_controllers[] list in VarCtl.cpp, so
// adding or editing a controlled variable only touches one file.
//
// If your real Main.h has other, unrelated declarations beyond what was
// shown here, keep those -- only these specific ones were removed.