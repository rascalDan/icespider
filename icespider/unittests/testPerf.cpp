#include <benchmark/benchmark.h>
#include <cgiRequestBase.h>
#include <core.h>
#include <definedDirs.h>
#include <fstream>

#define BENCHMARK_CAPTURE_LITERAL(Name, Value) BENCHMARK_CAPTURE(Name, Value, Value);

class TestRequest : public IceSpider::CgiRequestBase {
public:
	TestRequest(IceSpider::Core * c, const EnvArray env) : IceSpider::CgiRequestBase(c, env) { }

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
class CharPtrPtrArray : public std::vector<const char *> {
public:
	explicit CharPtrPtrArray(const std::filesystem::path & p)
	{
		std::ifstream f(p);
		for (std::string line; getline(f, line);) {
			lines.emplace_back(line);
		}
		std::transform(lines.begin(), lines.end(), std::back_inserter(*this), [](const auto & s) {
			return s.c_str();
		});
	}

private:
	std::vector<std::string> lines;
};

class CoreFixture : public IceSpider::CoreWithDefaultRouter, public benchmark::Fixture { };

BENCHMARK_F(CoreFixture, script_name_root)(benchmark::State & state)
{
	CharPtrPtrArray env(rootDir / "fixtures/env1");
	for (auto _ : state) {
		TestRequest r(this, env);
	}
}

BENCHMARK_F(CoreFixture, is_secure)(benchmark::State & state)
{
	CharPtrPtrArray env(rootDir / "fixtures/env1");
	TestRequest r(this, env);
	for (auto _ : state) {
		benchmark::DoNotOptimize(r.isSecure());
	}
}

BENCHMARK_F(CoreFixture, get_env_param)(benchmark::State & state)
{
	CharPtrPtrArray env(rootDir / "fixtures/env1");
	TestRequest r(this, env);
	for (auto _ : state) {
		benchmark::DoNotOptimize(r.getEnvStr("REMOTE_PORT"));
	}
}

BENCHMARK_F(CoreFixture, get_header_param)(benchmark::State & state)
{
	CharPtrPtrArray env(rootDir / "fixtures/env1");
	TestRequest r(this, env);
	for (auto _ : state) {
		benchmark::DoNotOptimize(r.getHeaderParamStr("user_agent"));
	}
}

BENCHMARK_F(CoreFixture, get_query_string_param)(benchmark::State & state)
{
	CharPtrPtrArray env(rootDir / "fixtures/env1");
	TestRequest r(this, env);
	for (auto _ : state) {
		benchmark::DoNotOptimize(r.getQueryStringParamStr("utm_source"));
	}
}

BENCHMARK_F(CoreFixture, get_cookie_param)(benchmark::State & state)
{
	CharPtrPtrArray env(rootDir / "fixtures/env1");
	TestRequest r(this, env);
	for (auto _ : state) {
		benchmark::DoNotOptimize(r.getQueryStringParamStr("utm_source"));
	}
}

namespace {
	void
	AcceptParse(benchmark::State & state, const std::string_view accept)
	{
		for (auto _ : state) {
			benchmark::DoNotOptimize(IceSpider::IHttpRequest::parseAccept(accept));
		}
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
