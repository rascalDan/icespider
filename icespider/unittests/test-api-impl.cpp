#include <iosfwd>
#include <string>
#include <test-api.h>

void
TestIceSpider::Ex::ice_print(std::ostream & s) const
{
	s << message;
}
