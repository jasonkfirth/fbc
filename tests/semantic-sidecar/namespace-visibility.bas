'' Project: FreeBASIC semantic sidecar tests
'' File: namespace-visibility.bas
'' Purpose: Exercise source import order, native propagation and scoped removal.
'' Responsibilities: Reopened namespaces, duplicate USING and lexical shadowing.
'' This file intentionally does NOT infer runtime execution from visibility.

namespace FirstSpace
    dim shared FirstValue as long
end namespace
namespace SecondSpace
    dim shared SecondValue as long
end namespace
namespace Isolated
    dim shared ScopedValue as long
end namespace
namespace Facade
    using FirstSpace
end namespace

sub ScopedProcedure()
    using Isolated
    print ScopedValue ' procedure use
end sub
scope
    using Isolated
    print ScopedValue ' block use
end scope
dim ScopedValue as long
print ScopedValue ' module use

using Facade
using Facade
namespace Facade
    using SecondSpace
end namespace
print FirstValue, SecondValue

'' end of namespace-visibility.bas
