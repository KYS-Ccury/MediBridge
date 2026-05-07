/****************************************************************************
** Meta object code from reading C++ file 'PillController.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../Backend/Controllers/PillController.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'PillController.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN10medibridge11controllers14PillControllerE_t {};
} // unnamed namespace

template <> constexpr inline auto medibridge::controllers::PillController::qt_create_metaobjectdata<qt_meta_tag_ZN10medibridge11controllers14PillControllerE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "medibridge::controllers::PillController",
        "is_loading_changed",
        "",
        "last_error_changed",
        "confidence_tier_changed",
        "tts_text_changed",
        "dur_result_changed",
        "last_request_id_changed",
        "identify_succeeded",
        "identify_failed",
        "error_code",
        "pool_loaded",
        "pool_load_failed",
        "pool_changed",
        "identify",
        "image_request_id",
        "utterance_request_id",
        "load_pool",
        "include_inactive",
        "add_to_pool",
        "item_code",
        "reg_method",
        "remove_from_pool",
        "pool_id",
        "reset_pool",
        "is_loading",
        "last_error",
        "confidence_tier",
        "tts_text",
        "dur_result",
        "last_request_id"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'is_loading_changed'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'last_error_changed'
        QtMocHelpers::SignalData<void()>(3, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'confidence_tier_changed'
        QtMocHelpers::SignalData<void()>(4, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'tts_text_changed'
        QtMocHelpers::SignalData<void()>(5, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'dur_result_changed'
        QtMocHelpers::SignalData<void()>(6, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'last_request_id_changed'
        QtMocHelpers::SignalData<void()>(7, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'identify_succeeded'
        QtMocHelpers::SignalData<void()>(8, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'identify_failed'
        QtMocHelpers::SignalData<void(const QString &)>(9, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 10 },
        }}),
        // Signal 'pool_loaded'
        QtMocHelpers::SignalData<void()>(11, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'pool_load_failed'
        QtMocHelpers::SignalData<void(const QString &)>(12, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 10 },
        }}),
        // Signal 'pool_changed'
        QtMocHelpers::SignalData<void()>(13, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'identify'
        QtMocHelpers::MethodData<void(const QString &, const QString &)>(14, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 15 }, { QMetaType::QString, 16 },
        }}),
        // Method 'load_pool'
        QtMocHelpers::MethodData<void(bool)>(17, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 18 },
        }}),
        // Method 'add_to_pool'
        QtMocHelpers::MethodData<void(const QString &, const QString &)>(19, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 20 }, { QMetaType::QString, 21 },
        }}),
        // Method 'remove_from_pool'
        QtMocHelpers::MethodData<void(int)>(22, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 23 },
        }}),
        // Method 'reset_pool'
        QtMocHelpers::MethodData<void()>(24, 2, QMC::AccessPublic, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
        // property 'is_loading'
        QtMocHelpers::PropertyData<bool>(25, QMetaType::Bool, QMC::DefaultPropertyFlags, 0),
        // property 'last_error'
        QtMocHelpers::PropertyData<QString>(26, QMetaType::QString, QMC::DefaultPropertyFlags, 1),
        // property 'confidence_tier'
        QtMocHelpers::PropertyData<QString>(27, QMetaType::QString, QMC::DefaultPropertyFlags, 2),
        // property 'tts_text'
        QtMocHelpers::PropertyData<QString>(28, QMetaType::QString, QMC::DefaultPropertyFlags, 3),
        // property 'dur_result'
        QtMocHelpers::PropertyData<QString>(29, QMetaType::QString, QMC::DefaultPropertyFlags, 4),
        // property 'last_request_id'
        QtMocHelpers::PropertyData<QString>(30, QMetaType::QString, QMC::DefaultPropertyFlags, 5),
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<PillController, qt_meta_tag_ZN10medibridge11controllers14PillControllerE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject medibridge::controllers::PillController::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10medibridge11controllers14PillControllerE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10medibridge11controllers14PillControllerE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN10medibridge11controllers14PillControllerE_t>.metaTypes,
    nullptr
} };

void medibridge::controllers::PillController::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<PillController *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->is_loading_changed(); break;
        case 1: _t->last_error_changed(); break;
        case 2: _t->confidence_tier_changed(); break;
        case 3: _t->tts_text_changed(); break;
        case 4: _t->dur_result_changed(); break;
        case 5: _t->last_request_id_changed(); break;
        case 6: _t->identify_succeeded(); break;
        case 7: _t->identify_failed((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 8: _t->pool_loaded(); break;
        case 9: _t->pool_load_failed((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 10: _t->pool_changed(); break;
        case 11: _t->identify((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 12: _t->load_pool((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 13: _t->add_to_pool((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 14: _t->remove_from_pool((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 15: _t->reset_pool(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (PillController::*)()>(_a, &PillController::is_loading_changed, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (PillController::*)()>(_a, &PillController::last_error_changed, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (PillController::*)()>(_a, &PillController::confidence_tier_changed, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (PillController::*)()>(_a, &PillController::tts_text_changed, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (PillController::*)()>(_a, &PillController::dur_result_changed, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (PillController::*)()>(_a, &PillController::last_request_id_changed, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (PillController::*)()>(_a, &PillController::identify_succeeded, 6))
            return;
        if (QtMocHelpers::indexOfMethod<void (PillController::*)(const QString & )>(_a, &PillController::identify_failed, 7))
            return;
        if (QtMocHelpers::indexOfMethod<void (PillController::*)()>(_a, &PillController::pool_loaded, 8))
            return;
        if (QtMocHelpers::indexOfMethod<void (PillController::*)(const QString & )>(_a, &PillController::pool_load_failed, 9))
            return;
        if (QtMocHelpers::indexOfMethod<void (PillController::*)()>(_a, &PillController::pool_changed, 10))
            return;
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast<bool*>(_v) = _t->is_loading(); break;
        case 1: *reinterpret_cast<QString*>(_v) = _t->last_error(); break;
        case 2: *reinterpret_cast<QString*>(_v) = _t->confidence_tier(); break;
        case 3: *reinterpret_cast<QString*>(_v) = _t->tts_text(); break;
        case 4: *reinterpret_cast<QString*>(_v) = _t->dur_result(); break;
        case 5: *reinterpret_cast<QString*>(_v) = _t->last_request_id(); break;
        default: break;
        }
    }
}

const QMetaObject *medibridge::controllers::PillController::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *medibridge::controllers::PillController::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10medibridge11controllers14PillControllerE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int medibridge::controllers::PillController::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 16)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 16;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 16)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 16;
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
void medibridge::controllers::PillController::is_loading_changed()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void medibridge::controllers::PillController::last_error_changed()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void medibridge::controllers::PillController::confidence_tier_changed()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void medibridge::controllers::PillController::tts_text_changed()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void medibridge::controllers::PillController::dur_result_changed()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void medibridge::controllers::PillController::last_request_id_changed()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void medibridge::controllers::PillController::identify_succeeded()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}

// SIGNAL 7
void medibridge::controllers::PillController::identify_failed(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 7, nullptr, _t1);
}

// SIGNAL 8
void medibridge::controllers::PillController::pool_loaded()
{
    QMetaObject::activate(this, &staticMetaObject, 8, nullptr);
}

// SIGNAL 9
void medibridge::controllers::PillController::pool_load_failed(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 9, nullptr, _t1);
}

// SIGNAL 10
void medibridge::controllers::PillController::pool_changed()
{
    QMetaObject::activate(this, &staticMetaObject, 10, nullptr);
}
QT_WARNING_POP
