#include "il2cpp_api.h"


bool LoadIl2CppApi(
    Il2CppApi& api,
    unsigned timeoutMs)
{
    api = {};

    HMODULE mod = nullptr;

    for (
        unsigned waited = 0;
        waited <= timeoutMs;
        waited += 100)
    {
        mod = GetModuleHandleW(
            L"GameAssembly.dll"
        );

        if (mod)
            break;

        Sleep(100);
    }

    if (!mod)
        return false;

    api.string_new =
        reinterpret_cast<PFN_string_new>(
            GetProcAddress(mod, "il2cpp_string_new")
            );


    api.domain_get =
        reinterpret_cast<PFN_domain_get>(
            GetProcAddress(
                mod,
                "il2cpp_domain_get"
            )
            );

    api.domain_get_assemblies =
        reinterpret_cast<PFN_domain_get_assemblies>(
            GetProcAddress(
                mod,
                "il2cpp_domain_get_assemblies"
            )
            );

    api.assembly_get_image =
        reinterpret_cast<PFN_assembly_get_image>(
            GetProcAddress(
                mod,
                "il2cpp_assembly_get_image"
            )
            );

    api.class_from_name =
        reinterpret_cast<PFN_class_from_name>(
            GetProcAddress(
                mod,
                "il2cpp_class_from_name"
            )
            );

    api.class_get_method_from_name =
        reinterpret_cast<PFN_class_get_method_from_name>(
            GetProcAddress(
                mod,
                "il2cpp_class_get_method_from_name"
            )
            );

    api.class_get_field_from_name =
        reinterpret_cast<PFN_class_get_field_from_name>(
            GetProcAddress(
                mod,
                "il2cpp_class_get_field_from_name"
            )
            );

    api.runtime_invoke =
        reinterpret_cast<PFN_runtime_invoke>(
            GetProcAddress(
                mod,
                "il2cpp_runtime_invoke"
            )
            );

    api.string_new =
        reinterpret_cast<PFN_string_new>(
            GetProcAddress(
                mod,
                "il2cpp_string_new"
            )
            );

    api.object_unbox =
        reinterpret_cast<PFN_object_unbox>(
            GetProcAddress(
                mod,
                "il2cpp_object_unbox"
            )
            );

    api.object_get_class =
        reinterpret_cast<PFN_object_get_class>(
            GetProcAddress(
                mod,
                "il2cpp_object_get_class"
            )
            );

    api.class_get_parent =
        reinterpret_cast<PFN_class_get_parent>(
            GetProcAddress(
                mod,
                "il2cpp_class_get_parent"
            )
            );

    api.class_get_name =
        reinterpret_cast<PFN_class_get_name>(
            GetProcAddress(
                mod,
                "il2cpp_class_get_name"
            )
            );

    api.class_get_type =
        reinterpret_cast<PFN_class_get_type>(
            GetProcAddress(
                mod,
                "il2cpp_class_get_type"
            )
            );

    api.type_get_object =
        reinterpret_cast<PFN_type_get_object>(
            GetProcAddress(
                mod,
                "il2cpp_type_get_object"
            )
            );

    api.class_from_type =
        reinterpret_cast<PFN_class_from_type>(
            GetProcAddress(
                mod,
                "il2cpp_class_from_type"
            )
            );

    api.field_get_type =
        reinterpret_cast<PFN_field_get_type>(
            GetProcAddress(
                mod,
                "il2cpp_field_get_type"
            )
            );

    api.field_get_value =
        reinterpret_cast<PFN_field_get_value>(
            GetProcAddress(
                mod,
                "il2cpp_field_get_value"
            )
            );

    api.field_set_value =
        reinterpret_cast<PFN_field_set_value>(
            GetProcAddress(
                mod,
                "il2cpp_field_set_value"
            )
            );

    api.array_length =
        reinterpret_cast<PFN_array_length>(
            GetProcAddress(
                mod,
                "il2cpp_array_length"
            )
            );

    api.thread_attach =
        reinterpret_cast<PFN_thread_attach>(
            GetProcAddress(
                mod,
                "il2cpp_thread_attach"
            )
            );

    api.thread_current =
        reinterpret_cast<PFN_thread_current>(
            GetProcAddress(
                mod,
                "il2cpp_thread_current"
            )
            );


    return
        api.domain_get &&
        api.domain_get_assemblies &&
        api.assembly_get_image &&
        api.class_from_name &&
        api.class_get_method_from_name &&
        api.class_get_field_from_name &&
        api.runtime_invoke &&
        api.string_new &&
        api.object_get_class &&
        api.class_get_parent &&
        api.class_get_type &&
        api.type_get_object &&
        api.field_get_value &&
        api.field_set_value &&
        api.array_length &&
        api.thread_attach &&
        api.string_new;
}


Il2CppClass FindIl2CppClass(
    const Il2CppApi& api,
    const char* nameSpace,
    const char* name)
{
    Il2CppDomain domain =
        api.domain_get();

    if (!domain)
        return nullptr;

    size_t count = 0;

    const Il2CppAssembly* assemblies =
        api.domain_get_assemblies(
            domain,
            &count
        );

    if (!assemblies)
        return nullptr;

    for (size_t i = 0; i < count; ++i)
    {
        Il2CppImage image =
            api.assembly_get_image(
                assemblies[i]
            );

        if (!image)
            continue;

        Il2CppClass klass =
            api.class_from_name(
                image,
                nameSpace,
                name
            );

        if (klass)
            return klass;
    }

    return nullptr;
}


Il2CppField FindFieldInHierarchy(
    const Il2CppApi& api,
    Il2CppClass klass,
    const char* name)
{
    while (klass)
    {
        Il2CppField field =
            api.class_get_field_from_name(
                klass,
                name
            );

        if (field)
            return field;

        klass =
            api.class_get_parent(
                klass
            );
    }

    return nullptr;
}


void** ArrayElements(void* array)
{
    return reinterpret_cast<void**>(
        reinterpret_cast<char*>(array)
        + 4 * sizeof(void*)
        );
}