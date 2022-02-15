#include <benchmark/benchmark.h>
#include <cgiRequestBase.h>
#include <core.h>
#include <definedDirs.h>
#include <fstream>

#define BENCHMARK_CAPTURE_LITERAL(Name, Value) BENCHMARK_CAPTURE(Name, Value, Value);

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
	CharPtrPtrArray env(rootDir / "fixtures/env1");
	for (auto _ : state) {
		TestRequest r(this, &env.front());
	}
}

BENCHMARK_F(CoreFixture, is_secure)(benchmark::State & state)
{
	CharPtrPtrArray env(rootDir / "fixtures/env1");
	TestRequest r(this, &env.front());
	for (auto _ : state) {
		benchmark::DoNotOptimize(r.isSecure());
	}
}

BENCHMARK_F(CoreFixture, get_env_param)(benchmark::State & state)
{
	CharPtrPtrArray env(rootDir / "fixtures/env1");
	TestRequest r(this, &env.front());
	for (auto _ : state) {
		benchmark::DoNotOptimize(r.getEnv("REMOTE_PORT"));
	}
}

BENCHMARK_F(CoreFixture, get_header_param)(benchmark::State & state)
{
	CharPtrPtrArray env(rootDir / "fixtures/env1");
	TestRequest r(this, &env.front());
	for (auto _ : state) {
		benchmark::DoNotOptimize(r.getHeaderParam("user_agent"));
	}
}

BENCHMARK_F(CoreFixture, get_query_string_param)(benchmark::State & state)
{
	CharPtrPtrArray env(rootDir / "fixtures/env1");
	TestRequest r(this, &env.front());
	for (auto _ : state) {
		benchmark::DoNotOptimize(r.getQueryStringParam("utm_source"));
	}
}

BENCHMARK_F(CoreFixture, get_cookie_param)(benchmark::State & state)
{
	CharPtrPtrArray env(rootDir / "fixtures/env1");
	TestRequest r(this, &env.front());
	for (auto _ : state) {
		benchmark::DoNotOptimize(r.getQueryStringParam("utm_source"));
	}
}

static void
AcceptParse(benchmark::State & state, const std::string_view accept)
{
	for (auto _ : state) {
		benchmark::DoNotOptimize(IceSpider::IHttpRequest::parseAccept(accept));
	}
}
BENCHMARK_CAPTURE_LITERAL(AcceptParse, "*/*");
BENCHMARK_CAPTURE_LITERAL(AcceptParse, "any/html");
BENCHMARK_CAPTURE_LITERAL(AcceptParse, "image/png, */*");
BENCHMARK_CAPTURE_LITERAL(AcceptParse, "image/png;q=0.1, */*");
BENCHMARK_CAPTURE_LITERAL(AcceptParse, "image/png;q=0.1, any/html");
BENCHMARK_CAPTURE_LITERAL(AcceptParse, "image/png;q=0.9, any/html;q=1.0, something/else;q=1.0");
BENCHMARK_CAPTURE_LITERAL(AcceptParse, "image/png;q=0.9, */*;q=1.0");
BENCHMARK_CAPTURE_LITERAL(AcceptParse, "text/*");
BENCHMARK_CAPTURE_LITERAL(AcceptParse, "text/html");

BENCHMARK_MAIN();
