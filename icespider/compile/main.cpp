#include <boost/program_options.hpp>
#include "routeCompiler.h"

namespace po = boost::program_options;

int
main(int c, char ** v)
{
	bool showHelp;
	IceSpider::Compile::RouteCompiler rc;
	boost::filesystem::path input, output;
	po::options_description opts("IceSpider compile options");
	opts.add_options()
		("input", po::value(&input), "Input .json file")
		("output", po::value(&output), "Output .cpp file")
		("include,I", po::value(&rc.searchPath)->composing(), "Search path")
		("help,h", po::value(&showHelp)->default_value(false)->zero_tokens(), "Help")
		;
	po::positional_options_description pod;
	pod.add("input", 1).add("output", 2);
	po::variables_map vm;
	po::store(po::command_line_parser(c, v).options(opts).positional(pod).run(), vm);
	po::notify(vm);

	if (showHelp || input.empty() || output.empty()) {
		std::cout << opts << std::endl;
		return 1;
	}

	rc.searchPath.push_back(input.parent_path());
	rc.compile(input, output);

	return 0;
}

