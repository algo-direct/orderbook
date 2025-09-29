find ./src/ ./boost_http_server/ -type f -iname '*.h' -o -iname '*.cpp' -o -iname '*.hpp' | while read line; do echo $line; clang-format --style=file -i $line ; done
