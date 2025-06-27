#include <iostream>
#include <string>
#include <string_view>
#include <filesystem>

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
      std::cout << input.substr(pos + 1) << '\n';
      continue;
    }

    if (first_token == "type")
    {
      pos = input.find(' ', pos);

      auto command = input.substr(pos + 1);

      if (command == "echo" or command == "exit" or command == "type")
      {
        std::cout << command << " is a shell builtin" << '\n';
        continue;
      }

      if (command == "cat" or command == "cp" or command == "mkdir")
      {
        std::cout << command << " is /bin/" << command << '\n';
        continue;
      }
      if (command == "my_exe")
      {
        std::string_view path(std::getenv("PATH"));
        std::cout << command << " is " << path.substr(0, path.find(':', 0)) << '/' << command << '\n';
        continue;
      }

      std::cout << command << ": not found" << '\n';
      continue;
    }

    if (input == "exit 0")
      return 0;

    std::string env_path(std::getenv("PATH"));
    auto bin_dir = env_path.substr(0, env_path.find(':', 0));
    std::string bin_path = bin_dir + "/" + first_token;
    if (std::filesystem::exists(bin_path))
    {
      std::system(input.c_str());
      continue;
    }

    std::cout << input << ": command not found" << '\n';
  }
}
