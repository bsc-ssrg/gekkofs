#ifndef IO_COMMANDS_HPP
#define IO_COMMANDS_HPP

// forward declare CLI::App
namespace CLI { struct App; }

void
mkdir_init(CLI::App& app);

void
open_init(CLI::App& app);

void
opendir_init(CLI::App& app);

void
read_init(CLI::App& app);

void
readdir_init(CLI::App& app);

void
rmdir_init(CLI::App& app);

void
stat_init(CLI::App& app);

void
write_init(CLI::App& app);

#endif // IO_COMMANDS_HPP
