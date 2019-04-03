#include <neos/neos.hpp>
#include <string>
#include <iostream>
#include <boost/program_options.hpp>
#include <neos/context.hpp>

using namespace std::literals::string_literals;

namespace
{
    bool command_arg_to_bool(const std::string& aArg)
    {
        if (aArg == "true" || aArg == "1" || aArg == "on")
            return true;
        else if (aArg == "false" || aArg == "0" || aArg == "off")
            return false;
        else
            throw std::runtime_error("invalid command argument(s)");
    }
}

void main_loop(boost::program_options::variables_map& aOptions)
{
    std::cout << "neos " << neos::NEOS_VERSION << std::endl;
    neos::context context;
    auto output_compilation_time = [&context]()
    {
        std::cout << "Compilation time: " <<
            std::chrono::duration_cast<std::chrono::microseconds>(context.compiler().end_time() - context.compiler().start_time()).count() / 1000.0 << "ms" << std::endl;
    };
    bool interactive = false;
    bool first = true;
    for (;;)
    {
        // todo: non-interactive mode (stdin goes straight to vm stdin)
        std::string line;
        if (first)
        {
            first = false;
            if (aOptions.count("s"))
                line = "s " + aOptions["s"].as<std::string>();
        }
        if (line.empty())
        {
            std::cout << (context.schema_loaded() ? context.running() || interactive ? context.schema().meta().name + ">" : context.schema().meta().name + "]" : "]");
            std::getline(std::cin, line);
        }
        std::vector<std::pair<std::string::const_iterator, std::string::const_iterator>> words;
        const std::string delimeters{ " " };
        neolib::tokens(line.cbegin(), line.cend(), delimeters.cbegin(), delimeters.cend(), words);
        if (words.empty())
            continue;
        const std::string command{ words[0].first, words[0].second };
        if (command.empty())
            continue;
        std::string parameters{ words.size() > 1 ? words[1].first : line.cend(), line.cend() };
        try
        {
            if (interactive)
                context.evaluate(line);
            else if (command == "?" || command == "h" || command == "help")
            {
                std::cout << "h(elp)\n"
                    << "s(chema) <path to language schema>       Load language scheme\n"
                    << "l(oad) <path to program>                 Load program\n"
                    << "r(un)                                    Run program\n"
                    << "![<expression>]                          Evaluate expression (enter interactive mode if expression omitted)\n"
                    << ":<input>                                 Input (as stdin)\n"
                    << "q(uit)                                   Quit neos\n"
                    << "lc                                       List loaded concept libraries\n"
                    << "ct <true|false>                          Compiler trace\n"
                    << "cte <true|false>                         Compiler trace emits\n"
                    << "m(etrics)                                Display metrics of running programs\n"
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
            {
                context.load_program(parameters);
                output_compilation_time();
            }
            else if (command == "r" || command == "run")
                context.run();
            else if (command[0] == '!')
            {
                if (!context.schema_loaded())
                    throw std::runtime_error("no schema loaded");
                std::string expression(std::next(words[0].first), line.cend());
                if (!expression.empty())
                {
                    context.evaluate(expression);
                    output_compilation_time();
                }
                else
                    interactive = true;
            }
            else if (command == "lc")
            {
                auto print_concept_library = [](auto const& self, const neos::language::i_concept_library& aConceptLibrary, uint32_t aDepth = 0) -> void
                {
                    std::cout << std::string(aDepth * 2, ' ') << "[" << aConceptLibrary.name() << "]";
                    if (aDepth == 0)
                        std::cout << " (" << aConceptLibrary.uri() << ")";
                    std::cout << std::endl;
                    for (auto const& conceptLibrary : aConceptLibrary.sublibraries())
                        self(self, *conceptLibrary.second(), aDepth + 1);
                };
                for (auto const& conceptLibrary : context.concept_libraries())
                    if (conceptLibrary.second()->depth() == 0)
                        print_concept_library(print_concept_library, *conceptLibrary.second());
            }
            else if (command == "ct")
            {
                if (words.size() == 2)
                    context.compiler().set_trace(command_arg_to_bool(std::string(words[1].first, words[1].second)));
                else
                    throw std::runtime_error("invalid command argument(s)");
            }
            else if (command == "cte")
            {
                if (words.size() == 2)
                    context.compiler().set_trace_emits(command_arg_to_bool(std::string(words[1].first, words[1].second)));
                else
                    throw std::runtime_error("invalid command argument(s)");
            }
            else if (command == "m" || command == "metrics")
                std::cout << context.metrics();
            else if (command == "q" || command == "quit")
                break;
            else
                throw std::runtime_error("unknown command '" + command + "'");
        }
        catch (const neos::language::schema::unresolved_references& e)
        {
            for (auto const& r : e.references)
            {
                std::cerr << "Error: " << context.schema_source().to_error_text(&r.first[0], "unresolved schema reference '" + r.first + "'") << std::endl;
            }
            throw;
        }
        catch (const neos::context::warning& e)
        {
            output_compilation_time();
            std::cerr << "Warning: " << e.what() << std::endl;
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

int main(int argc, char* argv[])
{
    try
    {
        boost::program_options::options_description optionsDescription{ "Allowed options" };
        optionsDescription.add_options()
            ("s", boost::program_options::value<std::string>()->implicit_value(""s), "schema");
        boost::program_options::variables_map options;
        boost::program_options::store(boost::program_options::parse_command_line(argc, argv, optionsDescription), options);
        main_loop(options);
    }
    catch (std::logic_error&)
    {
        throw;
    }
    catch (...)
    {
        return EXIT_FAILURE;
    }
}
