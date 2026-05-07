/****************************************************************************
** Meta object code from reading C++ file 'AuthController.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../Backend/Controllers/AuthController.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'AuthController.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.11.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN10medibridge11controllers14AuthControllerE_t {};
} // unnamed namespace

template <> constexpr inline auto medibridge::controllers::AuthController::qt_create_metaobjectdata<qt_meta_tag_ZN10medibridge11controllers14AuthControllerE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "medibridge::controllers::AuthController",
        "is_authenticated_changed",
        "",
        "current_user_id_changed",
        "current_user_email_changed",
        "current_user_name_changed",
        "is_loading_changed",
        "last_error_changed",
        "login_succeeded",
        "login_failed",
        "error_code",
        "signup_succeeded",
        "signup_failed",
        "logout_completed",
        "login",
        "email",
        "password",
        "signup",
        "user_name",
        "logout",
        "is_authenticated",
        "current_user_id",
        "current_user_email",
        "current_user_name",
        "is_loading",
        "last_error"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'is_authenticated_changed'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'current_user_id_changed'
        QtMocHelpers::SignalData<void()>(3, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'current_user_email_changed'
        QtMocHelpers::SignalData<void()>(4, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'current_user_name_changed'
        QtMocHelpers::SignalData<void()>(5, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'is_loading_changed'
        QtMocHelpers::SignalData<void()>(6, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'last_error_changed'
        QtMocHelpers::SignalData<void()>(7, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'login_succeeded'
        QtMocHelpers::SignalData<void()>(8, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'login_failed'
        QtMocHelpers::SignalData<void(const QString &)>(9, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 10 },
        }}),
        // Signal 'signup_succeeded'
        QtMocHelpers::SignalData<void()>(11, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'signup_failed'
        QtMocHelpers::SignalData<void(const QString &)>(12, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 10 },
        }}),
        // Signal 'logout_completed'
        QtMocHelpers::SignalData<void()>(13, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'login'
        QtMocHelpers::MethodData<void(const QString &, const QString &)>(14, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 15 }, { QMetaType::QString, 16 },
        }}),
        // Method 'signup'
        QtMocHelpers::MethodData<void(const QString &, const QString &, const QString &)>(17, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 15 }, { QMetaType::QString, 16 }, { QMetaType::QString, 18 },
        }}),
        // Method 'logout'
        QtMocHelpers::MethodData<void()>(19, 2, QMC::AccessPublic, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
        // property 'is_authenticated'
        QtMocHelpers::PropertyData<bool>(20, QMetaType::Bool, QMC::DefaultPropertyFlags, 0),
        // property 'current_user_id'
        QtMocHelpers::PropertyData<QString>(21, QMetaType::QString, QMC::DefaultPropertyFlags, 1),
        // property 'current_user_email'
        QtMocHelpers::PropertyData<QString>(22, QMetaType::QString, QMC::DefaultPropertyFlags, 2),
        // property 'current_user_name'
        QtMocHelpers::PropertyData<QString>(23, QMetaType::QString, QMC::DefaultPropertyFlags, 3),
        // property 'is_loading'
        QtMocHelpers::PropertyData<bool>(24, QMetaType::Bool, QMC::DefaultPropertyFlags, 4),
        // property 'last_error'
        QtMocHelpers::PropertyData<QString>(25, QMetaType::QString, QMC::DefaultPropertyFlags, 5),
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<AuthController, qt_meta_tag_ZN10medibridge11controllers14AuthControllerE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject medibridge::controllers::AuthController::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10medibridge11controllers14AuthControllerE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10medibridge11controllers14AuthControllerE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN10medibridge11controllers14AuthControllerE_t>.metaTypes,
    nullptr
} };

void medibridge::controllers::AuthController::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<AuthController *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->is_authenticated_changed(); break;
        case 1: _t->current_user_id_changed(); break;
        case 2: _t->current_user_email_changed(); break;
        case 3: _t->current_user_name_changed(); break;
        case 4: _t->is_loading_changed(); break;
        case 5: _t->last_error_changed(); break;
        case 6: _t->login_succeeded(); break;
        case 7: _t->login_failed((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 8: _t->signup_succeeded(); break;
        case 9: _t->signup_failed((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 10: _t->logout_completed(); break;
        case 11: _t->login((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 12: _t->signup((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[3]))); break;
        case 13: _t->logout(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (AuthController::*)()>(_a, &AuthController::is_authenticated_changed, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (AuthController::*)()>(_a, &AuthController::current_user_id_changed, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (AuthController::*)()>(_a, &AuthController::current_user_email_changed, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (AuthController::*)()>(_a, &AuthController::current_user_name_changed, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (AuthController::*)()>(_a, &AuthController::is_loading_changed, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (AuthController::*)()>(_a, &AuthController::last_error_changed, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (AuthController::*)()>(_a, &AuthController::login_succeeded, 6))
            return;
        if (QtMocHelpers::indexOfMethod<void (AuthController::*)(const QString & )>(_a, &AuthController::login_failed, 7))
            return;
        if (QtMocHelpers::indexOfMethod<void (AuthController::*)()>(_a, &AuthController::signup_succeeded, 8))
            return;
        if (QtMocHelpers::indexOfMethod<void (AuthController::*)(const QString & )>(_a, &AuthController::signup_failed, 9))
            return;
        if (QtMocHelpers::indexOfMethod<void (AuthController::*)()>(_a, &AuthController::logout_completed, 10))
            return;
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast<bool*>(_v) = _t->is_authenticated(); break;
        case 1: *reinterpret_cast<QString*>(_v) = _t->current_user_id(); break;
        case 2: *reinterpret_cast<QString*>(_v) = _t->current_user_email(); break;
        case 3: *reinterpret_cast<QString*>(_v) = _t->current_user_name(); break;
        case 4: *reinterpret_cast<bool*>(_v) = _t->is_loading(); break;
        case 5: *reinterpret_cast<QString*>(_v) = _t->last_error(); break;
        default: break;
        }
    }
}

const QMetaObject *medibridge::controllers::AuthController::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *medibridge::controllers::AuthController::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10medibridge11controllers14AuthControllerE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int medibridge::controllers::AuthController::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 14)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 14;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 14)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 14;
    }
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    }
    return _id;
}

// SIGNAL 0
void medibridge::controllers::AuthController::is_authenticated_changed()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void medibridge::controllers::AuthController::current_user_id_changed()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void medibridge::controllers::AuthController::current_user_email_changed()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void medibridge::controllers::AuthController::current_user_name_changed()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void medibridge::controllers::AuthController::is_loading_changed()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void medibridge::controllers::AuthController::last_error_changed()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void medibridge::controllers::AuthController::login_succeeded()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}

// SIGNAL 7
void medibridge::controllers::AuthController::login_failed(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 7, nullptr, _t1);
}

// SIGNAL 8
void medibridge::controllers::AuthController::signup_succeeded()
{
    QMetaObject::activate(this, &staticMetaObject, 8, nullptr);
}

// SIGNAL 9
void medibridge::controllers::AuthController::signup_failed(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 9, nullptr, _t1);
}

// SIGNAL 10
void medibridge::controllers::AuthController::logout_completed()
{
    QMetaObject::activate(this, &staticMetaObject, 10, nullptr);
}
QT_WARNING_POP
