#include <neos/neos.hpp>
#include <neos/context.hpp>
#include <string>
#include <iostream>

using namespace std::literals::string_literals;

int main()
{
    std::cout << "neos " << neos::NEOS_VERSION << std::endl;
    neos::context context;
    for (;;)
    {
        // todo: non-interactive mode (stdin goes straight to vm stdin)
        std::cout << (context.schema_loaded() ? context.running() ? context.schema().meta().name + ">" : context.schema().meta().name + "]" : "]");
        std::string line;
        std::getline(std::cin, line);
        std::vector<std::pair<std::string::const_iterator, std::string::const_iterator>> words;
        const std::string delimeters{ " " };
        neolib::tokens(line.cbegin(), line.cend(), delimeters.cbegin(), delimeters.cend(), words);
        if (words.empty())
            continue;
        const std::string command{ words[0].first, words[0].second };
        if (command.empty())
            continue;
        const std::string parameters{ words.size() > 1 ? words[1].first : line.cend(), line.cend() };
        try
        {
            if (command == "?" || command == "h" || command == "help")
            {
                std::cout << "h(elp)\n"
                    << "s(chema) <path to language schema>       Load language scheme\n"
                    << "l(oad) <path to program>                 Load program\n"
                    << "r(un)                                    Run program\n"
                    << "q(uit)                                   Quit neos\n"
                    << "lc                                       List loaded concept libraries\n"
                    << "m(etrics)                                Display metrics of running programs\n"
                    << ":<input>\n"
                    << std::endl;
            }
            else if (command == "s" || command == "schema")
            {
                context.load_schema(parameters);
                std::cout << "Language: " << context.schema().meta().description + "\nVersion: " + context.schema().meta().version << std::endl;
                if (!context.schema().meta().copyright.empty())
                    std::cout << context.schema().meta().copyright << std::endl;
            }
            else if (command == "l" || command == "load")
                context.load_program(parameters);
            else if (command == "r" || command == "run")
                context.run();
            else if (command == "q" || command == "quit")
                break;
            else if (command == "lc")
            {
                for (auto const& conceptLibrary : context.concept_libraries())
                    std::cout << "[" << conceptLibrary->name() << "]: " << conceptLibrary->description() << " (" << conceptLibrary->uri() << ")" << std::endl;
            }
            else if (command == "m" || command == "metrics")
                std::cout << context.metrics();
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error: " << e.what() << std::endl;
        }
        catch (...)
        {
            std::cerr << "Unknown error" << std::endl;
        }
    }
}
