/****************************************************************************
** Meta object code from reading C++ file 'HistoryController.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../Backend/Controllers/HistoryController.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'HistoryController.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN10medibridge11controllers17HistoryControllerE_t {};
} // unnamed namespace

template <> constexpr inline auto medibridge::controllers::HistoryController::qt_create_metaobjectdata<qt_meta_tag_ZN10medibridge11controllers17HistoryControllerE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "medibridge::controllers::HistoryController",
        "is_loading_changed",
        "",
        "last_error_changed",
        "total_count_changed",
        "record_succeeded",
        "record_failed",
        "error_code",
        "list_loaded",
        "list_load_failed",
        "record",
        "item_code",
        "quantity",
        "memo",
        "load_list",
        "from_date",
        "to_date",
        "page",
        "page_size",
        "is_loading",
        "last_error",
        "total_count"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'is_loading_changed'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'last_error_changed'
        QtMocHelpers::SignalData<void()>(3, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'total_count_changed'
        QtMocHelpers::SignalData<void()>(4, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'record_succeeded'
        QtMocHelpers::SignalData<void()>(5, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'record_failed'
        QtMocHelpers::SignalData<void(const QString &)>(6, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 7 },
        }}),
        // Signal 'list_loaded'
        QtMocHelpers::SignalData<void()>(8, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'list_load_failed'
        QtMocHelpers::SignalData<void(const QString &)>(9, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 7 },
        }}),
        // Method 'record'
        QtMocHelpers::MethodData<void(const QString &, int, const QString &)>(10, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 11 }, { QMetaType::Int, 12 }, { QMetaType::QString, 13 },
        }}),
        // Method 'load_list'
        QtMocHelpers::MethodData<void(const QString &, const QString &, int, int)>(14, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 15 }, { QMetaType::QString, 16 }, { QMetaType::Int, 17 }, { QMetaType::Int, 18 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
        // property 'is_loading'
        QtMocHelpers::PropertyData<bool>(19, QMetaType::Bool, QMC::DefaultPropertyFlags, 0),
        // property 'last_error'
        QtMocHelpers::PropertyData<QString>(20, QMetaType::QString, QMC::DefaultPropertyFlags, 1),
        // property 'total_count'
        QtMocHelpers::PropertyData<int>(21, QMetaType::Int, QMC::DefaultPropertyFlags, 2),
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<HistoryController, qt_meta_tag_ZN10medibridge11controllers17HistoryControllerE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject medibridge::controllers::HistoryController::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10medibridge11controllers17HistoryControllerE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10medibridge11controllers17HistoryControllerE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN10medibridge11controllers17HistoryControllerE_t>.metaTypes,
    nullptr
} };

void medibridge::controllers::HistoryController::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<HistoryController *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->is_loading_changed(); break;
        case 1: _t->last_error_changed(); break;
        case 2: _t->total_count_changed(); break;
        case 3: _t->record_succeeded(); break;
        case 4: _t->record_failed((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 5: _t->list_loaded(); break;
        case 6: _t->list_load_failed((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 7: _t->record((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[3]))); break;
        case 8: _t->load_list((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[3])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[4]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (HistoryController::*)()>(_a, &HistoryController::is_loading_changed, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (HistoryController::*)()>(_a, &HistoryController::last_error_changed, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (HistoryController::*)()>(_a, &HistoryController::total_count_changed, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (HistoryController::*)()>(_a, &HistoryController::record_succeeded, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (HistoryController::*)(const QString & )>(_a, &HistoryController::record_failed, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (HistoryController::*)()>(_a, &HistoryController::list_loaded, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (HistoryController::*)(const QString & )>(_a, &HistoryController::list_load_failed, 6))
            return;
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast<bool*>(_v) = _t->is_loading(); break;
        case 1: *reinterpret_cast<QString*>(_v) = _t->last_error(); break;
        case 2: *reinterpret_cast<int*>(_v) = _t->total_count(); break;
        default: break;
        }
    }
}

const QMetaObject *medibridge::controllers::HistoryController::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *medibridge::controllers::HistoryController::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10medibridge11controllers17HistoryControllerE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int medibridge::controllers::HistoryController::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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
        _id -= 3;
    }
    return _id;
}

// SIGNAL 0
void medibridge::controllers::HistoryController::is_loading_changed()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void medibridge::controllers::HistoryController::last_error_changed()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void medibridge::controllers::HistoryController::total_count_changed()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void medibridge::controllers::HistoryController::record_succeeded()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void medibridge::controllers::HistoryController::record_failed(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 4, nullptr, _t1);
}

// SIGNAL 5
void medibridge::controllers::HistoryController::list_loaded()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void medibridge::controllers::HistoryController::list_load_failed(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 6, nullptr, _t1);
}
QT_WARNING_POP
