#include <boost/program_options.hpp>
namespace po = boost::program_options;

int
main(int c, char ** v)
{
	std::string input, output;
	po::options_description opts("IceSpider compile options");
	opts.add_options()
		("input", po::value(&input), "Input .json file")
		("output", po::value(&output), "Output .cpp file")
		;
	po::positional_options_description pod;
	pod.add("input", 1).add("output", 2);
	po::variables_map vm;
	po::store(po::command_line_parser(c, v).options(opts).positional(pod).run(), vm);
	po::notify(vm);
	return 0;
}

