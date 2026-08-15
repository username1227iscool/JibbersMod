#pragma once

#include "VarCtl.h"
#include "Backend/il2cpp_api.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <string>

struct ControllerConfig;

class Controller
{
public:
    // Configuration
    void Configure(const ControllerConfig& config);

    // State
    bool Found() const;
    void ClearLocatedObject();

    // Runtime
    bool Locate();
    void CacheOriginal();
    void Tick();
    void Shutdown();

    // Values
    VarCtl::Vector3 ReadVector3();
    bool WriteVector3(const VarCtl::Vector3& value);

    // Invalidation
    void InvalidateField();
    void InvalidateTransform();

    // --------------------------------------------------------------------
    // CONFIG
    // --------------------------------------------------------------------

    std::string displayName;
    std::string id;
    std::string fieldName;

    VarCtl::ControllerKind kind =
        VarCtl::ControllerKind::Field;

    VarCtl::ValueType type =
        VarCtl::ValueType::Float;

    VarCtl::TransformProperty transformProperty =
        VarCtl::TransformProperty::Position;

    float multiplier = 1.0f;

    float overrideFloat = 0.0f;
    int overrideInt = 0;
    bool overrideBool = false;

    VarCtl::Vector3 overrideVector3{ 0.0f, 0.0f, 0.0f };

    float lowerLimit = 0.0f;
    float upperLimit = 1.0f;

    bool showSlider = false;

    // --------------------------------------------------------------------
    // STATE
    // --------------------------------------------------------------------

    std::atomic<bool> active{ false };

    void* object = nullptr;
    Il2CppField field = nullptr;

    Il2CppObject gameObject = nullptr;
    Il2CppObject transform = nullptr;

    bool ready = false;
    bool found = false;
    bool originalCached = false;
    bool touched = false;

    // --------------------------------------------------------------------
    // ORIGINAL VALUES
    // --------------------------------------------------------------------

    VarCtl::Vector3 originalVector3{ 0.0f, 0.0f, 0.0f };
    VarCtl::Vector3 lastTargetVector3{ 0.0f, 0.0f, 0.0f };

    float originalFloat = 0.0f;
    int originalInt = 0;
    bool originalBool = false;

    float lastTargetFloat = 0.0f;
    int lastTargetInt = 0;
    bool lastTargetBool = false;

    // --------------------------------------------------------------------
    // SCANNING
    // --------------------------------------------------------------------

    DWORD nextScanTick = 0;
};