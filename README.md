***WARNING! THIS PIECE OF SOFTWARE IS STILL IN DEVELOPMENT***
# boiling
Boiling is a boilerplate project structure generator that easily bootstraps any project that uses a supported programming language. The full list of programming languages supported is written in this README file.

## Config
You can copy the `config` directory inside the project root folder into your `.config` directory and then modify your config from there.

Licenses must have `[[Year]]` and `[[Name]]` placeholders for the current year and your config name accordingly.
```conf
[Core]
name=John Smith
license=./license/MIT
gitrepo=true

[Language]
name=c
bin=./build
src=./src

[Language]
name=cpp
bin=./build
src=./src

[Language]
name=py
src=./src
```

## Contributing
The application is made for my personal projects and for my project needs, but if anyone wants to help me in developing it or just wants to do add some features to use the application on daily basis, they're welcome to do so.
