"""FreeBASIC runtime tests; File: notification-server.py.
Purpose: Record real libnotify delivery on a private D-Bus session.
Responsibilities: Minimal freedesktop service, replies and exact argument logs.
This fixture does not display notifications or run on a user's desktop bus.
"""
import ctypes as C
import ctypes.util
import json
from pathlib import Path
import sys

gio = C.CDLL(C.util.find_library('gio-2.0'))
glib = C.CDLL(C.util.find_library('glib-2.0'))


def bind(library, symbol, result, *args):
    function = getattr(library, symbol)
    function.restype = result
    function.argtypes = args
    return function


pointer = C.c_void_p
bus_get = bind(gio, 'g_bus_get_sync', pointer, C.c_int, pointer, pointer)
node_new = bind(gio, 'g_dbus_node_info_new_for_xml', pointer, C.c_char_p, pointer)
register = bind(gio, 'g_dbus_connection_register_object', C.c_uint,
                pointer, C.c_char_p, pointer, pointer, pointer, pointer, pointer)
own = bind(gio, 'g_bus_own_name_on_connection', C.c_uint,
           pointer, C.c_char_p, C.c_uint, pointer, pointer, pointer, pointer)
child = bind(glib, 'g_variant_get_child_value', pointer, pointer, C.c_size_t)
string = bind(glib, 'g_variant_get_string', C.c_char_p, pointer, pointer)
uint32 = bind(glib, 'g_variant_get_uint32', C.c_uint32, pointer)
unref = bind(glib, 'g_variant_unref', None, pointer)
parse = bind(glib, 'g_variant_parse', pointer, pointer, C.c_char_p, pointer, pointer, pointer)
reply = bind(gio, 'g_dbus_method_invocation_return_value', None, pointer, pointer)
loop_new = bind(glib, 'g_main_loop_new', pointer, pointer, C.c_int)
loop_run = bind(glib, 'g_main_loop_run', None, pointer)
log = Path(sys.argv[1])
assert not log.exists()

XML = b'''<node><interface name="org.freedesktop.Notifications">
<method name="GetCapabilities"><arg direction="out" type="as"/></method>
<method name="GetServerInformation"><arg direction="out" type="s"/>
<arg direction="out" type="s"/><arg direction="out" type="s"/><arg direction="out" type="s"/></method>
<method name="Notify"><arg type="s" direction="in"/><arg type="u" direction="in"/>
<arg type="s" direction="in"/><arg type="s" direction="in"/><arg type="s" direction="in"/>
<arg type="as" direction="in"/><arg type="a{sv}" direction="in"/><arg type="i" direction="in"/>
<arg type="u" direction="out"/></method>
<method name="CloseNotification"><arg type="u" direction="in"/></method>
</interface></node>'''


def get_text(parameters, index):
    value = child(parameters, index)
    result = string(value, None).decode('utf-8')
    unref(value)
    return result


CALLBACK = C.CFUNCTYPE(None, pointer, C.c_char_p, C.c_char_p, C.c_char_p,
                      C.c_char_p, pointer, pointer, pointer)


@CALLBACK
def method(connection, sender, path, interface, method_name, parameters, invocation, user):
    name = method_name.decode()
    row = {'method': name}
    if name == 'GetCapabilities':
        response = b"(['body'],)" if len(sys.argv) > 2 and sys.argv[2] == 'plain' else b"(['body', 'body-markup'],)"
    elif name == 'GetServerInformation':
        response = b"('FreeBASIC test', 'FreeBASIC', '1', '1.2')"
    elif name == 'Notify':
        value = child(parameters, 1)
        row.update(app=get_text(parameters, 0), replaces_id=uint32(value),
                   title=get_text(parameters, 3), body=get_text(parameters, 4))
        unref(value)
        response = b'(uint32 1,)'
    elif name == 'CloseNotification':
        response = b'()'
    else:
        raise AssertionError(name)
    with log.open('a', encoding='utf-8') as stream:
        stream.write(json.dumps(row, ensure_ascii=False) + '\n')
    variant = parse(None, response, None, None, None)
    assert variant
    reply(invocation, variant)


class VTable(C.Structure):
    _fields_ = [('method', CALLBACK), ('get', pointer), ('set', pointer),
                ('padding', pointer * 8)]


class Node(C.Structure):
    _fields_ = [('references', C.c_int), ('path', C.c_char_p), ('interfaces', C.POINTER(pointer))]


table = VTable(method, None, None, (pointer * 8)())
connection = bus_get(2, None, None)  # G_BUS_TYPE_SESSION
assert connection
node = node_new(XML, None)
assert node
info = C.cast(node, C.POINTER(Node)).contents.interfaces[0]
assert register(connection, b'/org/freedesktop/Notifications', info, C.byref(table), None, None, None)

READY = C.CFUNCTYPE(None, pointer, C.c_char_p, pointer)


@READY
def acquired(connection, name, user):
    print('NOTIFY_SERVER_READY', flush=True)


assert own(connection, b'org.freedesktop.Notifications', 0, acquired, None, None, None)
loop_run(loop_new(None, False))
# end of notification-server.py
