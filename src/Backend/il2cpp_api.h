#pragma once

#include <windows.h>
#include <cstdint>
#include <cstddef>


// ============================================================================
// IL2CPP OPAQUE TYPES
// ============================================================================
//
// These are intentionally opaque handles.
// We use void* so the rest of the project does not need IL2CPP headers.
//

typedef void* Il2CppDomain;
typedef void* Il2CppAssembly;
typedef void* Il2CppImage;
typedef void* Il2CppClass;
typedef void* Il2CppMethod;
typedef void* Il2CppField;
typedef void* Il2CppObject;
typedef void* Il2CppException;
typedef void* Il2CppType;
typedef void* Il2CppReflectionType;


// ============================================================================
// FUNCTION TYPES
// ============================================================================

// il2cpp_domain_get
typedef Il2CppDomain(*PFN_domain_get)();


// il2cpp_domain_get_assemblies
//
// IL2CPP returns an array of assembly pointers.
// The returned array itself is not modified by us.
//
typedef const Il2CppAssembly** (*PFN_domain_get_assemblies)(
    Il2CppDomain domain,
    size_t* size
    );


// il2cpp_assembly_get_image
typedef Il2CppImage(*PFN_assembly_get_image)(
    Il2CppAssembly assembly
    );


// il2cpp_class_from_name
typedef Il2CppClass(*PFN_class_from_name)(
    Il2CppImage image,
    const char* namespaze,
    const char* name
    );


// il2cpp_class_get_method_from_name
typedef Il2CppMethod(*PFN_class_get_method_from_name)(
    Il2CppClass klass,
    const char* name,
    int argsCount
    );


// il2cpp_class_get_field_from_name
typedef Il2CppField(*PFN_class_get_field_from_name)(
    Il2CppClass klass,
    const char* name
    );


// il2cpp_runtime_invoke
typedef Il2CppObject(*PFN_runtime_invoke)(
    Il2CppMethod method,
    void* obj,
    void** params,
    Il2CppException* exc
    );


// il2cpp_object_unbox
typedef void* (*PFN_object_unbox)(
    Il2CppObject obj
    );


// il2cpp_object_get_class
typedef Il2CppClass(*PFN_object_get_class)(
    Il2CppObject obj
    );


// il2cpp_class_get_parent
typedef Il2CppClass(*PFN_class_get_parent)(
    Il2CppClass klass
    );


// il2cpp_class_get_name
typedef const char* (*PFN_class_get_name)(
    Il2CppClass klass
    );


// il2cpp_class_get_type
typedef const Il2CppType* (*PFN_class_get_type)(
    Il2CppClass klass
    );


// il2cpp_type_get_object
typedef Il2CppReflectionType* (*PFN_type_get_object)(
    const Il2CppType* type
    );


// il2cpp_class_from_type
typedef Il2CppClass(*PFN_class_from_type)(
    const Il2CppType* type
    );


// il2cpp_field_get_type
typedef const Il2CppType* (*PFN_field_get_type)(
    Il2CppField field
    );


// il2cpp_field_get_value
typedef void(*PFN_field_get_value)(
    Il2CppObject obj,
    Il2CppField field,
    void* value
    );


// il2cpp_field_set_value
typedef void(*PFN_field_set_value)(
    Il2CppObject obj,
    Il2CppField field,
    void* value
    );


// il2cpp_array_length
typedef uint32_t(*PFN_array_length)(
    void* array
    );


// il2cpp_thread_attach
typedef void(*PFN_thread_attach)(
    Il2CppDomain domain
    );


// il2cpp_thread_current
typedef void* (*PFN_thread_current)();


// il2cpp_string_new
typedef Il2CppObject(*PFN_string_new)(
    const char* str
    );


// ============================================================================
// IL2CPP API
// ============================================================================

struct Il2CppApi
{
    PFN_domain_get
        domain_get{};

    PFN_domain_get_assemblies
        domain_get_assemblies{};

    PFN_assembly_get_image
        assembly_get_image{};

    PFN_class_from_name
        class_from_name{};

    PFN_class_get_method_from_name
        class_get_method_from_name{};

    PFN_class_get_field_from_name
        class_get_field_from_name{};

    PFN_runtime_invoke
        runtime_invoke{};

    PFN_object_unbox
        object_unbox{};

    PFN_object_get_class
        object_get_class{};

    PFN_class_get_parent
        class_get_parent{};

    PFN_class_get_name
        class_get_name{};

    PFN_class_get_type
        class_get_type{};

    PFN_type_get_object
        type_get_object{};

    PFN_class_from_type
        class_from_type{};

    PFN_field_get_type
        field_get_type{};

    PFN_field_get_value
        field_get_value{};

    PFN_field_set_value
        field_set_value{};

    PFN_array_length
        array_length{};

    PFN_thread_attach
        thread_attach{};

    PFN_thread_current
        thread_current{};

    PFN_string_new
        string_new{};
};


// ============================================================================
// HELPERS
// ============================================================================

bool LoadIl2CppApi(
    Il2CppApi& api,
    unsigned timeoutMs
);


Il2CppClass FindIl2CppClass(
    const Il2CppApi& api,
    const char* nameSpace,
    const char* name
);


Il2CppField FindFieldInHierarchy(
    const Il2CppApi& api,
    Il2CppClass klass,
    const char* name
);


void** ArrayElements(
    void* array
);