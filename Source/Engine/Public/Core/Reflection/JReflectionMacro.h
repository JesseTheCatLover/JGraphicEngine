//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include "Core/Reflection/JReflectionRegistrar.h"

#define JREFLECT_TYPE(Type)                                       \
    void _JRegister_##Type(JReflectionRegistrar&);                \
    namespace {                                                   \
        struct _JAutoRegister_##Type {                            \
            _JAutoRegister_##Type() {                             \
                using SelfType = Type;                            \
                using BaseType = typename Type::Super;            \
                JReflectionRegistrar registrar(                   \
                    #Type, typeid(SelfType), typeid(BaseType));   \
                RETypeRegistry::SetDefaultFactory<SelfType>();    \
                _JRegister_##Type(registrar);                     \
            }                                                     \
        };                                                        \
        static _JAutoRegister_##Type _JAutoRegister_Instance_##Type; \
    }                                                             \
    void _JRegister_##Type(JReflectionRegistrar& registrar)       \
    {                                                             \
        using SelfType = Type;                                    \
        (void)registrar;


#define JREFLECT_TYPE_CTOR(Type, ...)                             \
    void _JRegister_##Type(JReflectionRegistrar&);                \
    namespace {                                                   \
        struct _JAutoRegister_##Type {                            \
            _JAutoRegister_##Type() {                             \
                using SelfType = Type;                            \
                using BaseType = typename Type::Super;            \
                JReflectionRegistrar registrar(                   \
                    #Type, typeid(SelfType), typeid(BaseType));   \
                RETypeRegistry::SetFactory<SelfType>(             \
                    []() -> JCoreObject* {                        \
                        return new SelfType(__VA_ARGS__);         \
                    }                                             \
                );                                                \
                _JRegister_##Type(registrar);                     \
            }                                                     \
        };                                                        \
        static _JAutoRegister_##Type _JAutoRegister_Instance_##Type; \
    }                                                             \
    void _JRegister_##Type(JReflectionRegistrar& registrar)       \
    {                                                             \
        using SelfType = Type;                                    \
        (void)registrar;


#define JREFLECT_ABSTRACT_TYPE(Type)                              \
    void _JRegister_##Type(JReflectionRegistrar&);                \
    namespace {                                                   \
        struct _JAutoRegister_##Type {                            \
            _JAutoRegister_##Type() {                             \
                using SelfType = Type;                            \
                using BaseType = typename Type::Super;            \
                JReflectionRegistrar registrar(                   \
                    #Type, typeid(SelfType), typeid(BaseType));   \
                RETypeRegistry::ClearFactory<SelfType>();         \
                _JRegister_##Type(registrar);                     \
            }                                                     \
        };                                                        \
        static _JAutoRegister_##Type _JAutoRegister_Instance_##Type; \
    }                                                             \
    void _JRegister_##Type(JReflectionRegistrar& registrar)       \
    {                                                             \
        using SelfType = Type;                                    \
        (void)registrar;


#define J_REFLECT_PROPERTY(Field, NameStr, MetaPack)              \
        registrar.AddProperty<SelfType>(NameStr, &SelfType::Field, MetaPack)

#define JPROPERTY(Field, ...)                                     \
        J_REFLECT_PROPERTY(Field, #Field, J_META_PACK(__VA_ARGS__))