/****************************************************************************
** Meta object code from reading C++ file 'PhoneLinkController.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../Backend/Controllers/PhoneLinkController.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'PhoneLinkController.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN10medibridge11controllers19PhoneLinkControllerE_t {};
} // unnamed namespace

template <> constexpr inline auto medibridge::controllers::PhoneLinkController::qt_create_metaobjectdata<qt_meta_tag_ZN10medibridge11controllers19PhoneLinkControllerE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "medibridge::controllers::PhoneLinkController",
        "is_connected_changed",
        "",
        "device_serial_changed",
        "device_state_changed",
        "reverse_active_changed",
        "last_error_changed",
        "phone_ready",
        "on_device_changed",
        "serial",
        "state",
        "retry_connect",
        "retry_reverse",
        "is_connected",
        "device_serial",
        "device_state",
        "reverse_active",
        "last_error"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'is_connected_changed'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'device_serial_changed'
        QtMocHelpers::SignalData<void()>(3, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'device_state_changed'
        QtMocHelpers::SignalData<void()>(4, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'reverse_active_changed'
        QtMocHelpers::SignalData<void()>(5, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'last_error_changed'
        QtMocHelpers::SignalData<void()>(6, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'phone_ready'
        QtMocHelpers::SignalData<void()>(7, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'on_device_changed'
        QtMocHelpers::SlotData<void(const QString &, const QString &)>(8, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 9 }, { QMetaType::QString, 10 },
        }}),
        // Method 'retry_connect'
        QtMocHelpers::MethodData<void()>(11, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'retry_reverse'
        QtMocHelpers::MethodData<void()>(12, 2, QMC::AccessPublic, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
        // property 'is_connected'
        QtMocHelpers::PropertyData<bool>(13, QMetaType::Bool, QMC::DefaultPropertyFlags, 0),
        // property 'device_serial'
        QtMocHelpers::PropertyData<QString>(14, QMetaType::QString, QMC::DefaultPropertyFlags, 1),
        // property 'device_state'
        QtMocHelpers::PropertyData<QString>(15, QMetaType::QString, QMC::DefaultPropertyFlags, 2),
        // property 'reverse_active'
        QtMocHelpers::PropertyData<bool>(16, QMetaType::Bool, QMC::DefaultPropertyFlags, 3),
        // property 'last_error'
        QtMocHelpers::PropertyData<QString>(17, QMetaType::QString, QMC::DefaultPropertyFlags, 4),
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<PhoneLinkController, qt_meta_tag_ZN10medibridge11controllers19PhoneLinkControllerE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject medibridge::controllers::PhoneLinkController::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10medibridge11controllers19PhoneLinkControllerE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10medibridge11controllers19PhoneLinkControllerE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN10medibridge11controllers19PhoneLinkControllerE_t>.metaTypes,
    nullptr
} };

void medibridge::controllers::PhoneLinkController::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<PhoneLinkController *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->is_connected_changed(); break;
        case 1: _t->device_serial_changed(); break;
        case 2: _t->device_state_changed(); break;
        case 3: _t->reverse_active_changed(); break;
        case 4: _t->last_error_changed(); break;
        case 5: _t->phone_ready(); break;
        case 6: _t->on_device_changed((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 7: _t->retry_connect(); break;
        case 8: _t->retry_reverse(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (PhoneLinkController::*)()>(_a, &PhoneLinkController::is_connected_changed, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (PhoneLinkController::*)()>(_a, &PhoneLinkController::device_serial_changed, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (PhoneLinkController::*)()>(_a, &PhoneLinkController::device_state_changed, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (PhoneLinkController::*)()>(_a, &PhoneLinkController::reverse_active_changed, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (PhoneLinkController::*)()>(_a, &PhoneLinkController::last_error_changed, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (PhoneLinkController::*)()>(_a, &PhoneLinkController::phone_ready, 5))
            return;
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast<bool*>(_v) = _t->is_connected(); break;
        case 1: *reinterpret_cast<QString*>(_v) = _t->device_serial(); break;
        case 2: *reinterpret_cast<QString*>(_v) = _t->device_state(); break;
        case 3: *reinterpret_cast<bool*>(_v) = _t->reverse_active(); break;
        case 4: *reinterpret_cast<QString*>(_v) = _t->last_error(); break;
        default: break;
        }
    }
}

const QMetaObject *medibridge::controllers::PhoneLinkController::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *medibridge::controllers::PhoneLinkController::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10medibridge11controllers19PhoneLinkControllerE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int medibridge::controllers::PhoneLinkController::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 9)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 9;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 9)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 9;
    }
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    }
    return _id;
}

// SIGNAL 0
void medibridge::controllers::PhoneLinkController::is_connected_changed()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void medibridge::controllers::PhoneLinkController::device_serial_changed()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void medibridge::controllers::PhoneLinkController::device_state_changed()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void medibridge::controllers::PhoneLinkController::reverse_active_changed()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void medibridge::controllers::PhoneLinkController::last_error_changed()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void medibridge::controllers::PhoneLinkController::phone_ready()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}
QT_WARNING_POP
