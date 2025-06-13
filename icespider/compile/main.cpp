#include "routeCompiler.h"
#include <array>
#include <boost/algorithm/string/compare.hpp>
#include <boost/algorithm/string/find_iterator.hpp>
#include <boost/algorithm/string/finder.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>
#include <compileTimeFormatter.h>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace po = boost::program_options;
using namespace AdHoc::literals;

namespace {
	std::string
	defaultPostProcessor()
	{
		constexpr std::array<const std::pair<std::string_view, std::string_view>, 1> POST_PROCESSORS {{
				{"clang-format", "-i"},
		}};
		const std::string_view pathEnv {getenv("PATH") ?: "/usr/bin"};
		for (const auto & [cmd, opts] : POST_PROCESSORS) {
			for (const auto & path : std::ranges::split_view(pathEnv, ':')) {
				if (std::filesystem::exists(std::filesystem::path(path.begin(), path.end()) / cmd)) {
					return "%? %?"_fmt(cmd, opts);
				}
			}
		}
		return {};
	}
}

int
main(int argc, char ** argv)
{
	bool showHelp = false;
	IceSpider::Compile::RouteCompiler routeCompiler;
	std::filesystem::path input, output;
	std::string post;
	po::options_description opts("IceSpider compile options");
	// clang-format off
	opts.add_options()
		("input", po::value(&input), "Input .json file")
		("output", po::value(&output), "Output .cpp file")
		("include,I", po::value(&routeCompiler.searchPath)->composing(), "Search path")
		("post,p", po::value(&post)->default_value(defaultPostProcessor()), "Post-process command")
		("help,h", po::value(&showHelp)->default_value(false)->zero_tokens(), "Help")
		;
	// clang-format on
	po::positional_options_description pod;
	pod.add("input", 1).add("output", 2);
	po::variables_map varMap;
	po::store(po::command_line_parser(argc, argv).options(opts).positional(pod).run(), varMap);
	po::notify(varMap);

	if (showHelp || input.empty() || output.empty()) {
		// LCOV_EXCL_START
		std::cout << opts << '\n';
		return 1;
		// LCOV_EXCL_STOP
	}

	routeCompiler.searchPath.push_back(input.parent_path());
	routeCompiler.compile(input, output);

	if (!post.empty()) {
		auto outputh = std::filesystem::path {output}.replace_extension(".h");
		return system("%? %? %?"_fmt(post, output, outputh).c_str());
	}

	return 0;
}
