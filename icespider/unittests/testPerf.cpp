#include <benchmark/benchmark.h>
#include <cgiRequestBase.h>
#include <core.h>
#include <definedDirs.h>
#include <fstream>

class TestRequest : public IceSpider::CgiRequestBase {
public:
	TestRequest(IceSpider::Core * c, char ** env) : IceSpider::CgiRequestBase(c, env)
	{
		initialize();
	}

	std::ostream &
	getOutputStream() const override
	{
		return out;
	}

	// LCOV_EXCL_START we never actually read or write anything here
	std::istream &
	getInputStream() const override
	{
		return std::cin;
	}
	// LCOV_EXCL_STOP

	// NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
	mutable std::stringstream out;
};

// NOLINTNEXTLINE(hicpp-special-member-functions)
class CharPtrPtrArray : public std::vector<char *> {
public:
	explicit CharPtrPtrArray(const std::filesystem::path & p)
	{
		reserve(40);
		std::ifstream f(p);
		while (f.good()) {
			std::string line;
			std::getline(f, line, '\n');
			push_back(strdup(line.c_str()));
		}
		push_back(nullptr);
	}

	~CharPtrPtrArray()
	{
		for (const auto & e : *this) {
			// NOLINTNEXTLINE(hicpp-no-malloc)
			free(e);
		}
	}
};

class CoreFixture : public IceSpider::CoreWithDefaultRouter, public benchmark::Fixture {
};

BENCHMARK_F(CoreFixture, script_name_root)(benchmark::State & state)
{
	CharPtrPtrArray env(XSTR(ROOT) "/fixtures/env1");
	for (auto _ : state) {
		TestRequest r(this, &env.front());
	}
}

BENCHMARK_MAIN();
