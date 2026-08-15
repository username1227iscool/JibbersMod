#pragma once

#include "VarCtl.h"
#include "Backend/il2cpp_api.h"

#include <cstddef>

namespace VarCtlUnity
{
    // ========================================================================
    // LIFETIME / INITIALIZATION
    // ========================================================================

    bool Initialize();
    bool IsReady();
    void Reset();

    // ========================================================================
    // IL2CPP
    // ========================================================================

    Il2CppApi& Api();

    void EnsureThreadAttached();

    // ========================================================================
    // POINTER / OBJECT VALIDITY
    // ========================================================================

    bool PointerLooksValid(const void* pointer);
    bool InstanceAlive(Il2CppObject instance);

    // ========================================================================
    // UNITY OBJECTS
    // ========================================================================

    Il2CppObject FindGameObject(const char* path);
    Il2CppObject GetTransform(Il2CppObject gameObject);

    // ========================================================================
    // TRANSFORM VALUES
    // ========================================================================

    VarCtl::Vector3 GetTransformVector(
        Il2CppObject transform,
        VarCtl::TransformProperty property
    );

    bool SetTransformVector(
        Il2CppObject transform,
        VarCtl::TransformProperty property,
        const VarCtl::Vector3& value
    );

    // ========================================================================
    // FIELD SEARCH
    // ========================================================================

    Il2CppField FindFieldByClass(
        Il2CppClass klass,
        const char* fieldName
    );

    bool FieldTypeMatches(
        Il2CppField field,
        VarCtl::ValueType requestedType
    );

    bool FindFieldAndObject(
        const char* fieldName,
        VarCtl::ValueType requestedType,
        void*& outObject,
        Il2CppField& outField
    );
}