#pragma once
#include <windows.h>
#include <cstdint>


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

typedef Il2CppDomain(*PFN_domain_get)();
typedef const Il2CppAssembly*(*PFN_domain_get_assemblies)(Il2CppDomain, size_t*);
typedef Il2CppImage(*PFN_assembly_get_image)(Il2CppAssembly);
typedef Il2CppClass(*PFN_class_from_name)(Il2CppImage, const char*, const char*);
typedef Il2CppMethod(*PFN_class_get_method_from_name)(Il2CppClass, const char*, int);
typedef Il2CppField(*PFN_class_get_field_from_name)(Il2CppClass, const char*);
typedef Il2CppObject(*PFN_runtime_invoke)(Il2CppMethod, void*, void**, Il2CppException*);
typedef void*(*PFN_object_unbox)(Il2CppObject);
typedef Il2CppClass(*PFN_object_get_class)(Il2CppObject);
typedef Il2CppClass(*PFN_class_get_parent)(Il2CppClass);
typedef const char*(*PFN_class_get_name)(Il2CppClass);
typedef const Il2CppType*(*PFN_class_get_type)(Il2CppClass);
typedef Il2CppReflectionType*(*PFN_type_get_object)(const Il2CppType*);
typedef Il2CppClass(*PFN_class_from_type)(const Il2CppType*);
typedef const Il2CppType*(*PFN_field_get_type)(Il2CppField);
typedef void(*PFN_field_get_value)(Il2CppObject, Il2CppField, void*);
typedef void(*PFN_field_set_value)(Il2CppObject, Il2CppField, void*);
typedef uint32_t(*PFN_array_length)(void*);
typedef void(*PFN_thread_attach)(Il2CppDomain);
typedef void*(*PFN_thread_current)();

struct Il2CppApi
{
    PFN_domain_get domain_get;
    PFN_domain_get_assemblies domain_get_assemblies;
    PFN_assembly_get_image assembly_get_image;
    PFN_class_from_name class_from_name;
    PFN_class_get_method_from_name class_get_method_from_name;
    PFN_class_get_field_from_name class_get_field_from_name;
    PFN_runtime_invoke runtime_invoke;
    PFN_object_unbox object_unbox;
    PFN_object_get_class object_get_class;
    PFN_class_get_parent class_get_parent;
    PFN_class_get_name class_get_name;
    PFN_class_get_type class_get_type;
    PFN_type_get_object type_get_object;
    PFN_class_from_type class_from_type;
    PFN_field_get_type field_get_type;
    PFN_field_get_value field_get_value;
    PFN_field_set_value field_set_value;
    PFN_array_length array_length;
    PFN_thread_attach thread_attach;
    PFN_thread_current thread_current;
};

bool LoadIl2CppApi(Il2CppApi& api, unsigned timeoutMs);

Il2CppClass FindIl2CppClass(const Il2CppApi& api, const char* nameSpace, const char* name);

Il2CppField FindFieldInHierarchy(const Il2CppApi& api, Il2CppClass klass, const char* name);

void** ArrayElements(void* array);
