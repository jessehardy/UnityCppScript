﻿#include "cpsInternal.h"
#include "cpsTypes.h"
#include "cpsUtils.h"
#include <vector>
#include <algorithm>


const char* cpsType::getName() const
{
    if (!mtype) { return nullptr; }
    return mono_type_get_name(mtype);
}

cpsClass cpsType::getClass() const
{
    return mono_type_get_class(mtype);
}



const char* cpsField::getName() const
{
    if (!mfield) { return nullptr; }
    return mono_field_get_name(mfield);
}

cpsType cpsField::getType() const
{
    return mono_field_get_type(mfield);
}

int cpsField::getOffset() const
{
    return mono_field_get_offset(mfield);
}

void cpsField::getValueImpl(cpsObject obj, void *p) const
{
    if (!mfield) { return; }
    mono_field_get_value(obj, mfield, p);
}

void cpsField::setValueImpl(cpsObject obj, const void *p)
{
    if (!mfield) { return; }
    mono_field_set_value(obj, mfield, (void*)p);
}


const char* cpsProperty::getName() const
{
    if (!mproperty) { return nullptr; }
    return mono_property_get_name(mproperty);
}

cpsMethod cpsProperty::getGetter() const
{
    if (!mproperty) { return nullptr; }
    return mono_property_get_get_method(mproperty);
}

cpsMethod cpsProperty::getSetter() const
{
    if (!mproperty) { return nullptr; }
    return mono_property_get_set_method(mproperty);
}



const char* cpsMethod::getName() const
{
    if (!mmethod) { return nullptr; }
    return mono_method_get_name(mmethod);
}

cpsObject cpsMethod::invoke(cpsObject obj, void **args)
{
    if (!mmethod) { return nullptr; }
    return mono_runtime_invoke(mmethod, obj, args, nullptr);
}

int cpsMethod::getParamCount() const
{
    if (!mmethod) { return -1; }
    void *sig = mono_method_signature(mmethod);
    return mono_signature_get_param_count(sig);
}

cpsType cpsMethod::getReturnType() const
{
    if (!mmethod) { return nullptr; }
    void *sig = mono_method_signature(mmethod);
    return mono_signature_get_return_type(sig);
}

void cpsMethod::eachArgTypes(const std::function<void(cpsType&)>& f) const
{
    if (!mmethod) { return; }
    void *sig = mono_method_signature(mmethod);
    void *mt = nullptr;
    gpointer iter = nullptr;
    while (mt = mono_signature_get_params(sig, &iter)) {
        cpsType miot = mt;
        f(miot);
    }
}


const char* cpsClass::getName() const
{
    if (!mclass) { return nullptr; }
    return mono_class_get_name(mclass);
}

cpsType cpsClass::getType() const
{
    if (!mclass) { return nullptr; }
    return mono_class_get_type(mclass);
}

cpsField cpsClass::findField(const char *name) const
{
    if (!mclass) { return nullptr; }
    return mono_class_get_field_from_name(mclass, name);
}

cpsProperty cpsClass::findProperty(const char *name) const
{
    if (!mclass) { return nullptr; }
    return mono_class_get_property_from_name(mclass, name);
}

cpsMethod cpsClass::findMethod(const char *name, int num_args) const
{
    if (!mclass) { return nullptr; }
    return mono_class_get_method_from_name(mclass, name, num_args);
}

void cpsClass::eachFields(const std::function<void(cpsField&)> &f)
{
    void *field;
    gpointer iter = nullptr;
    while ((field = mono_class_get_fields(mclass, &iter))) {
        cpsField mf = field;
        f(mf);
    }
}

void cpsClass::eachProperties(const std::function<void(cpsProperty&)> &f)
{
    void *prop;
    gpointer iter = nullptr;
    while ((prop = mono_class_get_properties(mclass, &iter))) {
        cpsProperty mp = prop;
        f(mp);
    }
}

void cpsClass::eachMethods(const std::function<void(cpsMethod&)> &f)
{
    void *method;
    gpointer iter = nullptr;
    while ((method = mono_class_get_methods(mclass, &iter))) {
        cpsMethod mm = method;
        f(mm);
    }
}

void cpsClass::eachFieldsUpwards(const std::function<void(cpsField&, cpsClass&)> &f)
{
    cpsClass c = mclass;
    do {
        void *field;
        gpointer iter = nullptr;
        while (field = mono_class_get_fields(c, &iter)) {
            cpsField m = field;
            f(m, c);
        }
        c = c.getParent();
    } while (c);
}

void cpsClass::eachPropertiesUpwards(const std::function<void(cpsProperty&, cpsClass&)> &f)
{
    cpsClass c = mclass;
    do {
        void *prop;
        gpointer iter = nullptr;
        while (prop = mono_class_get_properties(c, &iter)) {
            cpsProperty m = prop;
            f(m, c);
        }
        c = c.getParent();
    } while (c);
}

void cpsClass::eachMethodsUpwards(const std::function<void(cpsMethod&, cpsClass&)> &f)
{
    cpsClass c = mclass;
    do {
        void *method;
        gpointer iter = nullptr;
        while (method = mono_class_get_methods(c, &iter)) {
            cpsMethod m = method;
            f(m, c);
        }
        c = c.getParent();
    } while (c);
}


cpsClass cpsClass::getParent() const
{
    return mono_class_get_parent(mclass);
}


cpsClass cpsObject::getClass() const
{
    return mono_object_get_class(mobj);
}

cpsField cpsObject::findField(const char *name) const
{
    return getClass().findField(name);
}

cpsProperty cpsObject::findProperty(const char *name) const
{
    return getClass().findProperty(name);
}

cpsMethod cpsObject::findMethod(const char *name) const
{
    return getClass().findMethod(name);
}


cpsAPI void cpsAddMethod(const char *name, void *addr)
{
    mono_add_internal_call(name, addr);
}

cpsExport void cpsCoreInitialize()
{
    static bool s_is_first = true;
    if (s_is_first) {
        s_is_first = false;

        std::string path;
        path.resize(1024 * 64);

#ifdef _WIN32
        // add plugin directory to PATH environment variable to load dependent dlls 
        DWORD ret = ::GetEnvironmentVariableA("PATH", &path[0], path.size());
        path.resize(ret);
        {
            char path_to_this_module[MAX_PATH + 1];
            HMODULE mod = 0;
            ::GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCSTR)&cpsCoreInitialize, &mod);
            DWORD size = ::GetModuleFileNameA(mod, path_to_this_module, sizeof(path_to_this_module));
            for (int i = size - 1; i >= 0; --i) {
                if (path_to_this_module[i] == '\\') {
                    path_to_this_module[i] = '\0';
                    break;
                }
            }
            path += ";";
            path += path_to_this_module;
        }
        ::SetEnvironmentVariableA("PATH", path.c_str());

#if !defined(cpsWithoutDbgHelp)
        ::SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_DEBUG);
        ::SymInitialize(::GetCurrentProcess(), NULL, TRUE);
#endif

#elif defined(__APPLE__)

#endif
    }
}