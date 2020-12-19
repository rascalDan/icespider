#include "util.h"

namespace foo ::bar {
	class really;
}

static_assert(type_names<int>::name() == "int");
static_assert(type_names<int>::namespaces() == 0);
static_assert(TypeName<int>::str() == "int");
static_assert(type_names<foo::bar::really>::name() == "foo::bar::really");
static_assert(type_names<foo::bar::really>::namespaces() == 2);
static_assert(TypeName<foo::bar::really>::str() == "foo.bar.really");
