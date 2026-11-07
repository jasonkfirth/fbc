'' Project: FreeBASIC GObject binding tests
'' File: glib-type-callbacks.bas
'' Purpose: Exercise the type-definition macros through the native GObject API.
'' Responsibilities: Check callback forwarding, private storage and type identity.
'' This file intentionally does NOT create a GUI or load image data.
#lang "fb"
#include once "glib-object.bi"

type AuditStatic
	parent as GObject
	value as long
end type
type AuditStaticClass
	parent as GObjectClass
end type
type AuditStaticPrivate
	marker as long
end type
type AuditAbstract
	parent as GObject
end type
type AuditAbstractClass
	parent as GObjectClass
end type
type AuditAbstractPrivate
	marker as long
end type
type AuditIfaceInterface
	parent as GTypeInterface
	marker as long
end type
type AuditDynamic
	parent as GObject
	value as long
end type
type AuditDynamicClass
	parent as GObjectClass
end type
type AuditModule
	parent as GTypeModule
end type
type AuditModuleClass
	parent as GTypeModuleClass
end type

dim shared as long static_classes, static_instances, static_finalized
dim shared as long interface_defaults, dynamic_classes, dynamic_instances
dim shared as long dynamic_finalized, module_loads, module_unloads

G_DEFINE_TYPE_WITH_PRIVATE(AuditStatic, audit_static, G_TYPE_OBJECT)
G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE(AuditAbstract, audit_abstract, G_TYPE_OBJECT)
G_DEFINE_INTERFACE(AuditIface, audit_iface, G_TYPE_OBJECT)
G_DEFINE_DYNAMIC_TYPE(AuditDynamic, audit_dynamic, G_TYPE_OBJECT)
G_DEFINE_TYPE(AuditModule, audit_module, G_TYPE_TYPE_MODULE)

extern "C"
	private sub audit_static_finalize(byval obj as GObject ptr)
		static_finalized += 1
		cptr(GObjectClass ptr, audit_static_parent_class)->finalize(obj)
	end sub

	sub audit_static_class_init(byval klass as AuditStaticClass ptr)
		static_classes += 1
		klass->parent.finalize = @audit_static_finalize
	end sub

	sub audit_static_init(byval self as AuditStatic ptr)
		static_instances += 1
		self->value = 42
		var priv = cptr(AuditStaticPrivate ptr, audit_static_get_instance_private(self))
		if priv = NULL then end 1
		priv->marker = 12345
	end sub

	sub audit_iface_default_init(byval klass as AuditIfaceInterface ptr)
		interface_defaults += 1
		klass->marker = 901
	end sub

	sub audit_abstract_class_init(byval klass as AuditAbstractClass ptr)
	end sub

	sub audit_abstract_init(byval self as AuditAbstract ptr)
		var priv = cptr(AuditAbstractPrivate ptr, audit_abstract_get_instance_private(self))
		if priv = NULL then end 16
		priv->marker = 54321
	end sub

	sub audit_dynamic_class_init(byval klass as AuditDynamicClass ptr)
		dynamic_classes += 1
	end sub

	sub audit_dynamic_class_finalize(byval klass as AuditDynamicClass ptr)
		dynamic_finalized += 1
	end sub

	sub audit_dynamic_init(byval self as AuditDynamic ptr)
		dynamic_instances += 1
		self->value = 73
	end sub

	private function audit_module_load(byval module as GTypeModule ptr) as gboolean
		module_loads += 1
		audit_dynamic_register_type(module)
		return TRUE
	end function

	private sub audit_module_unload(byval module as GTypeModule ptr)
		module_unloads += 1
	end sub

	sub audit_module_class_init(byval klass as AuditModuleClass ptr)
		klass->parent.load = @audit_module_load
		klass->parent.unload = @audit_module_unload
	end sub

	sub audit_module_init(byval self as AuditModule ptr)
	end sub
end extern

var static_type = audit_static_get_type()
if static_type = 0 orelse audit_static_get_type() <> static_type then end 2
for i as long = 1 to 3
	var obj = cptr(AuditStatic ptr, g_object_new(static_type, NULL))
	if obj = NULL then end 3
	var priv = cptr(AuditStaticPrivate ptr, audit_static_get_instance_private(obj))
	if obj->value <> 42 orelse priv->marker <> 12345 then end 4
	if g_type_check_instance_is_a(@obj->parent.g_type_instance, static_type) = FALSE then end 5
	g_object_unref(obj)
next
if static_classes <> 1 or static_instances <> 3 or static_finalized <> 3 then end 6

var interface_type = audit_iface_get_type()
if interface_type = 0 orelse audit_iface_get_type() <> interface_type then end 7
var iface = cptr(AuditIfaceInterface ptr, g_type_default_interface_ref(interface_type))
if iface = NULL orelse iface->marker <> 901 or interface_defaults <> 1 then end 8
g_type_default_interface_unref(iface)

var abstract_type = audit_abstract_get_type()
if abstract_type = 0 orelse g_type_test_flags(abstract_type, G_TYPE_FLAG_ABSTRACT) = FALSE then end 17
var abstract_class = g_type_class_ref(abstract_type)
if abstract_class = NULL then end 18
g_type_class_unref(abstract_class)

'' GTypeModule objects that register types must remain available for future
'' reloads. Keep this process-lifetime module alive after dropping its use count.
var module = cptr(GTypeModule ptr, g_object_new(audit_module_get_type(), NULL))
if module = NULL orelse audit_dynamic_get_type() <> 0 then end 9
g_type_module_set_name(module, "FreeBASIC callback contract")
if g_type_module_use(module) = FALSE then end 10
var dynamic_type = audit_dynamic_get_type()
if dynamic_type = 0 or module_loads <> 1 then end 11
for i as long = 1 to 3
	var obj = cptr(AuditDynamic ptr, g_object_new(dynamic_type, NULL))
	if obj = NULL orelse obj->value <> 73 then end 12
	g_object_unref(obj)
next
if dynamic_classes <> 1 or dynamic_instances <> 3 then end 13

'' GObject may retain a dynamic class in its cache. Independently dispatch the
'' exact finalizer signature to verify forwarding without assuming cache eviction.
dim finalizer as GClassFinalizeFunc = @audit_dynamic_class_intern_finalize
var klass = g_type_class_ref(dynamic_type)
if klass = NULL then end 14
var previous_finalized = dynamic_finalized
finalizer(klass, NULL)
if dynamic_finalized <> previous_finalized + 1 then end 15
g_type_class_unref(klass)
g_type_module_unuse(module)
print "GLIB_TYPE_CALLBACKS_PASS"

'' end of glib-type-callbacks.bas
