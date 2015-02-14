#include <string>
#include <sstream>

#include "cmd_args.h"
#include "data_sizes.h"

cli_options command_line_options;

void process_command_line(int argc, char **argv)
{
	for (int i = 0; i < argc; i++)
	{
		if (std::string(argv[i]) == "-k")
		{
			i++;

			std::stringstream ss;
			ss << std::hex << argv[i];
			ss >> command_line_options.start_key;
		}
		else if (std::string(argv[i]) == "-d")
		{
			i++;

			std::stringstream ss;
			ss << std::hex << argv[i];
			ss >> command_line_options.start_data;
		}
		else if (std::string(argv[i]) == "-c")
		{
			i++;

			std::stringstream ss;
			ss << std::hex << argv[i];
			ss >> command_line_options.iv_count;
		}
		else if (std::string(argv[i]) == "-e")
		{
			i++;

			std::stringstream ss;
			ss << std::hex << argv[i];
			ss >> command_line_options.vector_size;
		}
		else if (std::string(argv[i]) == "-a")
		{
			i++;
			if (std::string(argv[i]) == "individual")
			{
				command_line_options.attack = INDIVIDUAL;
			}
			else if (std::string(argv[i]) == "vector")
			{
				command_line_options.attack = VECTOR;
			}
			else if (std::string(argv[i]) == "vector2")
			{
				command_line_options.attack = VECTOR_CONSTANT_SIZE;
			}
			else if (std::string(argv[i]) == "hash")
			{
				command_line_options.attack = HASH;
			}
		}
		else if (std::string(argv[i]) == "-b")
		{
			command_line_options.big_registers = true;
		}
		else if (std::string(argv[i]) == "-s")
		{
			command_line_options.key_schedule = true;
		}
		else if (std::string(argv[i]) == "-f")
		{
			command_line_options.from_file = true;

			i++;
			command_line_options.filename = std::string(argv[i]);
		}
	}
}