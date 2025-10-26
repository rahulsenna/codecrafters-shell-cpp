#include <iostream>
#include <string>
#include <string_view>
#include <filesystem>
#include <ranges>
#include <format>
#include <unistd.h>
#include <stdlib.h>

#include <readline/readline.h>
#include <readline/history.h>
#include <cstring>
#include <vector>
#include <unordered_set>
#include <algorithm>
#include <fstream>

#include <sys/types.h>
#include <sys/wait.h>


#define raw_mode 0

#if raw_mode
#include <termios.h>
#include <vector>
#include <unordered_set>

void enable_raw_mode(termios &original)
{
  termios raw = original;
  raw.c_lflag &= ~(ICANON | ECHO); // disable canonical mode and echo
  tcsetattr(STDIN_FILENO, TCSANOW, &raw);
}
void disable_raw_mode(const termios &original)
{
  tcsetattr(STDIN_FILENO, TCSANOW, &original); // restore original settings
}
#endif

std::vector<std::string> builtins = {"echo", "cd", "exit", "pwd", "history", "type"};
std::vector<std::string> executables = builtins;
std::vector<std::string> env_paths;

void get_execuatables(std::string &env_path)
{
  std::unordered_set<std::string> seen(executables.begin(), executables.end()); // Avoid duplicates
  for (const auto path_range : env_paths)
  {
    std::string_view path_sv(path_range.begin(), path_range.end());
    if (path_sv.empty())
      continue;

    std::filesystem::path dir_path(path_sv);

    std::error_code ec;

    if (!std::filesystem::exists(dir_path, ec) || !std::filesystem::is_directory(dir_path, ec))
      continue;
    for (const auto &entry : std::filesystem::directory_iterator(dir_path, ec))
    {
      if (ec)
        break;

      if (!entry.is_regular_file(ec) || ec)
        continue;

      const auto &file_path = entry.path();

      auto perms = entry.status(ec).permissions();
      if (ec)
        continue;

      // Check for execute permission (owner, group, or others)
      bool is_executable = (perms & std::filesystem::perms::owner_exec) != std::filesystem::perms::none ||
                           (perms & std::filesystem::perms::group_exec) != std::filesystem::perms::none ||
                           (perms & std::filesystem::perms::others_exec) != std::filesystem::perms::none;

      if (is_executable)
      {
        std::string filename = file_path.filename().string();

        if (seen.insert(filename).second)
        {
          executables.push_back(std::move(filename));
        }
      }
    }
  }
}

char *command_generator(const char *text, int state)
{
  static int index, len;

  if (state == 0)
  {
    index = 0;
    len = std::strlen(text);
  }

  for (; index < executables.size(); index++)
  {
    if (std::strncmp(executables[index].c_str(), text, len) == 0)
    {
      return strdup(executables[index++].c_str());
    }
  }

  return nullptr;
}

char **command_completion(const char *text, int start, int end)
{
  if (start == 0)
  {
    char **matches = rl_completion_matches(text, command_generator);
    if (!matches)
      std::cout << "\x07" << std::flush; // ring the bell
    else
    {
      for (int i = 2; matches[i] != nullptr; i++) // if more than one matches (add one extra space)
      {
        auto modified = std::string(matches[i]) + "";
        matches[i] = strdup(modified.c_str());;
      }
    }
    return matches;
  }
  return nullptr;
}

void do_types(std::string input)
{
  int pos = input.find(' ', pos);
  pos = input.find(' ', pos);
  auto command = input.substr(pos + 1);

  if (std::ranges::contains(builtins, command))
  {
    std::cout << command << " is a shell builtin" << '\n';
    return;
  }

  for (const auto path : env_paths)
  {
    auto file_path = std::format("{}/{}", std::string_view(path), command);
    if (std::filesystem::exists(file_path))
    {
      auto p = std::filesystem::status(file_path).permissions();
      if ((p & std::filesystem::perms::owner_exec) != std::filesystem::perms::none ||
          (p & std::filesystem::perms::group_exec) != std::filesystem::perms::none)
      {
        std::cout << command << " is " << file_path << '\n';
        return;
      }
    }
  }

  std::cout << command << ": not found" << '\n';
}

void write_history_to_file(const char *filename, bool append = true)
{
  // write_history(filename);
  std::ofstream file = append ? std::ofstream(filename, std::ios::app) : std::ofstream(filename);

  if (!file.is_open())
  {
    std::cerr << "Error: Cannot open " << filename << std::endl;
    return;
  }
  for (int i = 1; i <= history_length; ++i)
  {
    HIST_ENTRY *entry = history_get(i);
    file << entry->line << std::endl;
  }
  file.close();
  clear_history();
}

void load_history_from_file(const char *filename)
{
  // read_history(filename);
  std::ifstream file(filename);
  if (!file.is_open())
  {
    std::cerr << "Error: Cannot open " << filename << std::endl;
    return;
  }
  std::string line;
  while (std::getline(file, line))
    add_history(line.c_str());

  file.close();
}

int main()
{
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  rl_attempted_completion_function = command_completion;
  std::string env_path(std::getenv("PATH"));
  env_paths = std::views::split(env_path, ':') | std::ranges::to<std::vector<std::string>>();

  get_execuatables(env_path);
  const char *hist = std::getenv("HISTFILE");
  if (hist)
    load_history_from_file(hist);

  while (1)
  {
#if raw_mode
    std::cout << "$ ";
    
    termios original;
    tcgetattr(STDIN_FILENO, &original);
    enable_raw_mode(original);
    std::string input;
    char ch;
    while (read(STDIN_FILENO, &ch, 1) == 1)
    {
      if (ch == '\n')
        break;
      
      if (ch == '\t') 
      {
        for (auto &cmd: executables)
        {
        	if (cmd.starts_with(input))
          {
            input = cmd + " ";
            std::cout << "\r$ " << input << std::flush;
          }
        }
        continue;
      }
      input += ch;
      std::cout << ch << std::flush;
    }
    std::cout << std::endl;
    disable_raw_mode(original);
#endif

    char* line = readline("$ ");
    if (line && *line)
    {
        add_history(line);
    }
    std::string input(line);

    auto pos = input.find(' ');
    auto first_token = input.substr(0, pos);
    if (input[0] == '\'')
    {
      pos = input.find('\'', 1);
      first_token = input.substr(1, pos - 1);
    }
    if (input[0] == '"')
    {
      pos = input.find('"', 1);
      first_token = input.substr(1, pos - 1);
    }

    int redirect_out = input.find('>');
    std::string_view outfile = "";
    if (redirect_out != -1)
        outfile = std::string_view(input.begin()+redirect_out+2, input.end());

    std::string pipe_program = "";
    if (redirect_out == -1)
    {
      redirect_out = input.find('|');
      if (redirect_out != -1)
      {
        pipe_program = std::string(input.begin()+redirect_out+2, input.end());
      }
    }

    if (first_token == "echo")
    { 
      auto line = input.substr(pos + 1, redirect_out != -1 ? redirect_out - pos - 2 : -1);
      bool in_single_quote = 0;
      bool in_double_quote = 0;
     
      std::string buffer;
      buffer.reserve(1024*10);
      int buf_i = 0;

      char prev = 0;
      for (int i = 0; i < line.length(); ++i)
      {
        char c = line[i];
        if (not in_double_quote and c == '\'')
        {
          in_single_quote = not in_single_quote;
          continue;
        }
        if (c == '"')
        {
          in_double_quote = not in_double_quote;
          continue;
        }
        if (c == ' ' and prev == ' ' and not(in_single_quote or in_double_quote))
        {
          continue;
        }
        if (c == '\\')
        {
          if (in_single_quote)
            buffer += c;
          
          c = line[++i];
        }
        buffer += c;
        prev = c;
      }
      buffer += '\n';
      if (not outfile.empty())
      {
        std::string mode = "w";
        if (input[redirect_out + 1] == '>')
        {
          mode = "a";
          outfile = outfile.substr(1);
        }
        FILE *out = fopen(outfile.data(), mode.c_str());
        if (input[redirect_out - 1] != '2') /* stdout */
          size_t written = fwrite(buffer.data(), 1, buffer.size(), out);
        else
          std::cout << buffer;

        fclose(out);
      } else if (not pipe_program.empty())
      {
        int pipefd[2];
        
        if (pipe(pipefd) == -1)
        {
          perror("pipe");
        	exit(EXIT_FAILURE);
        }
        int pipe_read_fd = pipefd[0];
        int pipe_write_fd = pipefd[1];
        pid_t writer_pid = fork();
        if (writer_pid == 0)
        { 
          dup2(pipe_write_fd, STDOUT_FILENO);
          close(pipe_read_fd);
          std::cout << buffer << std::flush;
          exit(0);
        }
        wait(0);
        pid_t reader_pid = fork();
        if (reader_pid == 0)
        { 
          dup2(pipe_read_fd, STDIN_FILENO);
          close(pipe_write_fd);
          std::system(pipe_program.c_str());
          exit(0);
        }
        close(pipe_write_fd);
        close(pipe_read_fd);
        wait(0);
        
      }
      else
        std::cout << buffer;
      continue;
    }

    if (first_token == "type")
    {
      do_types(input);
      continue;
    }

    if (first_token == "history")
    {
      auto args = std::views::split(input, ' ') | std::ranges::to<std::vector<std::string>>();
      int start_index = 1;
      if (args.size() == 2)
      {
        std::string_view num_str = args[1];
        start_index = history_length - std::stoi(num_str.data()) + 1;
      }
      else if (args.size() == 3 and args[1] == "-r")
      {
        load_history_from_file(args[2].c_str());
        continue;
      }
      else if (args.size() == 3 and (args[1] == "-w" or args[1] == "-a"))
      {
        write_history_to_file(args[2].c_str());
        continue;
      }
      for (int i = start_index; i <= history_length; ++i)
      {
        HIST_ENTRY *entry = history_get(i);
        std::cout << i << "\t" << entry->line << '\n';
      }
      continue;
    }

    if (input == "exit 0")
    {
      if (hist)
        write_history_to_file(hist, false);
      return 0;
    }      

    if (first_token == "cd")
    {
      auto target_dir = input.substr(3);
      if (target_dir == "~")
        target_dir = std::getenv("HOME");

      if (!std::filesystem::exists(target_dir))
      {
        // println("cd: {}: No such file or directory", target_dir);
        printf("cd: %s: No such file or directory\n", target_dir.data());
        continue;
      }

      if (chdir(target_dir.data()) == 0)
      {
        auto this_path = std::filesystem::current_path().string();
        if (this_path.starts_with("/private"))
        {
          this_path = this_path.substr(8);
        }
        setenv("PWD", this_path.c_str(), 1);
      }
      continue;
    }
    if (first_token == "echo" or first_token == "exit" or first_token == "type" or first_token == "pwd")
    {
      std::system(input.data());
      continue;
    }

    bool bin_exist = false;

    for (const auto word : env_paths)
    {
      auto bin_path = std::format("{}/{}", std::string_view(word), first_token);
      if (std::filesystem::exists(bin_path))
      {
        bin_exist = true;
        break;
      }
    }
    if (bin_exist)
    {
      std::system(input.data());
      continue;
    }

    std::cout << input << ": command not found" << '\n';
  }
}
