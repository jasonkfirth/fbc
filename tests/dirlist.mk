# Project: FreeBASIC test suite
# File: dirlist.mk
# Purpose: Select directories used by unit and dialect log tests.
# Responsibilities: Define the source tree for each supported language mode.
# This file intentionally does NOT contain: per-test compile or link rules.
#

# EDIT this file when new directories are added to the testing tree

DIRLIST_FB := \
pretest \
boolean \
comments \
command-sweep \
compound \
console \
const \
cpp \
crt \
data \
datetime \
dim \
expressions \
fbc-int \
file \
functions \
gfx \
interactive \
namespace \
numbers \
optimizations \
overload \
pointers \
pp \
quirk \
scopes \
serialization \
sfx \
string \
structs \
swap \
threads \
typedef \
udt-wstring \
udt-zstring \
virtual \
visibility \
wstring

DIRLIST_FBLITE := \
fblite

DIRLIST_QB := \
qb

DIRLIST_DEPRECATED := \
deprecated

# end of dirlist.mk
