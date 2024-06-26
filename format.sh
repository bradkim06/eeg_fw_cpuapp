#!/bin/bash

# 현재 폴더에서 build폴더와 lib폴더를 제외한 하위 모든 파일들을 clang-format을 적용해주는 쉘 스크립트입니다.

# clang-format을 적용할 파일의 확장자를 지정합니다.
EXTENSIONS=".cpp .h .hpp .c .cc .cxx .hh .hxx"

# 현재 폴더에서 build폴더와 lib폴더를 제외한 하위 모든 파일들을 순회하며 clang-format을 적용합니다.
for file in $(find . -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.hpp" -o -name "*.c" -o -name "*.cc" -o -name "*.cxx" -o -name "*.hh" -o -name "*.hxx" \) ! -path "./build/*" ! -path "./lib/*"); do
	# 파일의 확장자를 가져옵니다.
	extension="${file##*.}"
	# 확장자가 지정한 확장자들 중 하나와 일치하는 경우에만 clang-format을 적용합니다.
	if [[ $EXTENSIONS =~ .*$extension.* ]]; then
		# clang-format을 적용합니다.
		clang-format --verbose -i --style=file $file
	fi
done

# 모든 파일에 대해 clang-format을 적용한 후에는 적용한 파일의 개수를 출력합니다.
echo "clang-format applied to $(find . -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.hpp" -o -name "*.c" -o -name "*.cc" -o -name "*.cxx" -o -name "*.hh" -o -name "*.hxx" \) ! -path "./build/*" ! -path "./lib/*" | wc -l) files"
