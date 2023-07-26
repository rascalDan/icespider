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
		constexpr std::array<const std::pair<std::string_view, std::string_view>, 1> pps {{
				{"clang-format", "-i"},
		}};
		const std::string_view path {getenv("PATH") ?: "/usr/bin"};
		const auto pathBegin = make_split_iterator(path, first_finder(":", boost::is_equal()));
		for (const auto & [cmd, opts] : pps) {
			for (auto p = pathBegin; p != decltype(pathBegin) {}; ++p) {
				if (std::filesystem::exists(std::filesystem::path(p->begin(), p->end()) / cmd)) {
					return "%? %?"_fmt(cmd, opts);
				}
			}
		}
		return {};
	}
}

int
main(int c, char ** v)
{
	bool showHelp = false;
	IceSpider::Compile::RouteCompiler rc;
	std::filesystem::path input, output;
	std::string post;
	po::options_description opts("IceSpider compile options");
	// clang-format off
	opts.add_options()
		("input", po::value(&input), "Input .json file")
		("output", po::value(&output), "Output .cpp file")
		("include,I", po::value(&rc.searchPath)->composing(), "Search path")
		("post,p", po::value(&post)->default_value(defaultPostProcessor()), "Post-process command")
		("help,h", po::value(&showHelp)->default_value(false)->zero_tokens(), "Help")
		;
	// clang-format on
	po::positional_options_description pod;
	pod.add("input", 1).add("output", 2);
	po::variables_map vm;
	po::store(po::command_line_parser(c, v).options(opts).positional(pod).run(), vm);
	po::notify(vm);

	if (showHelp || input.empty() || output.empty()) {
		// LCOV_EXCL_START
		std::cout << opts << std::endl;
		return 1;
		// LCOV_EXCL_STOP
	}

	rc.searchPath.push_back(input.parent_path());
	rc.compile(input, output);

	if (!post.empty()) {
		auto outputh = std::filesystem::path {output}.replace_extension(".h");
		return system("%? %? %?"_fmt(post, output, outputh).c_str());
	}

	return 0;
}
