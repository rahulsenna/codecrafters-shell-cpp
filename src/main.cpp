#include <iostream>
#include <string>
#include <string_view>
#include <filesystem>
#include <ranges>
#include <format>
#include <unistd.h>
#include <stdlib.h>

int main() {
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  

  std::string input;
  while(1)
  {
    std::cout << "$ ";
    std::getline(std::cin, input);

    auto pos = input.find(' ');
    auto first_token = input.substr(0, pos);

    if (first_token == "echo")
    {
      auto line = input.substr(pos + 1);
      int single_quote_cnt = 0;
      bool in_single_quote = 0;

      int double_quote_cnt = 0;
      bool in_double_quote = 0;

      char prev = 0;
      for (auto c : line)
      {
        if (not in_double_quote and c == '\'')
        {
          single_quote_cnt++;
          in_single_quote = !(single_quote_cnt % 2 == 0);
          continue;
        }
        if (c == '"')
        {
          double_quote_cnt++;
          in_double_quote = !(double_quote_cnt % 2 == 0);
          continue;
        }
        if (c == ' ' and prev == ' ' and not (in_single_quote or in_double_quote)) 
        {
          	continue; 
        }

        putchar(c);
        prev = c;
      }
      std::cout << '\n';
      continue;
    }

    if (first_token == "type")
    {
      pos = input.find(' ', pos);

      auto command = input.substr(pos + 1);

      if (command == "echo" or command == "exit" or command == "type" or command == "pwd")
      {
        std::cout << command << " is a shell builtin" << '\n';
        continue;
      }

      if (command == "cat" or command == "cp" or command == "mkdir")
      {
        if (std::filesystem::exists("/usr/bin/cat"))
          std::cout << command << " is /usr/bin/" << command << '\n';
        else
          std::cout << command << " is /bin/" << command << '\n';
        continue;
      }
      if (command == "my_exe")
      {
        std::string env_path(std::getenv("PATH"));
        for (const auto word: std::views::split(env_path, ':'))
        { 
            auto file_path = std::format("{}/my_exe", std::string_view(word));
            if (std::filesystem::exists(file_path))
            {
              auto p = std::filesystem::status(file_path).permissions();
              if ((p & std::filesystem::perms::owner_exec) != std::filesystem::perms::none ||
                  (p & std::filesystem::perms::group_exec) != std::filesystem::perms::none)
              {
                std::cout << command << " is " << file_path << '\n';
                break;
              }
            }
        }
        continue;
      }

      std::cout << command << ": not found" << '\n';
      continue;
    }

    if (input == "exit 0")
      return 0;

    if (first_token == "cd")
    {
        auto target_dir = input.substr(3);
        if (target_dir  == "~")
        	target_dir = std::getenv("HOME");
        
        if (!std::filesystem::exists(target_dir))
        {
            // println("cd: {}: No such file or directory", target_dir);
            printf("cd: %s: No such file or directory\n", target_dir.c_str());
            continue;
        }

        if (chdir(target_dir.c_str()) == 0)
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
      std::system(input.c_str());
      continue;
    }
      

        
    bool bin_exist = false;
    std::string env_path(std::getenv("PATH"));
    for (const auto word: std::views::split(env_path, ':'))
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
      std::system(input.c_str());
      continue;
    }

    std::cout << input << ": command not found" << '\n';
  }
}
