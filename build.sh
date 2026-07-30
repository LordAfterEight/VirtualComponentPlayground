mkdir -p bin obj
source project.env

if [[ -z "${PROJECT_NAME}" ]]; then
    echo -e "\x1b[38;2;255;150;100mPROJECT_NAME\x1b[0m environment variable \x1b[38;2;255;255;100mis unset\x1b[0m."
    echo -e "Please run '\x1b[38;2;255;255;255mexport PROJECT_NAME=<name of project>\x1b[0m'"
    exit 1
else
    echo "Building ${PROJECT_NAME}..."
    error_happened=0
    rm obj/*
    while IFS= read -r -d '' file; do
        filename=$(basename "$file")
        name="${filename%.*}"
        printf "\x1b[38;2;100;255;100mCompiling\x1b[0m %s" "$filename"
        if error_output=$(g++ -std="$STD" $(pkg-config --cflags sdl3) -c "$file" -o "obj/${name}.o" 2>&1); then
            printf "\x1b[38;2;100;255;100m    OK\x1b[0m\n"
        else
            printf "\x1b[38;2;255;100;100m    ERR\x1b[0m\n"
            echo "$error_output"
            error_happened=1
            exit 1
        fi
    done < <(find src -type f -name "*.cpp" -print0)

    if [ $error_happened -eq 0 ]; then
        echo "Linking..."
        if g++ -std="$STD" obj/*.o -o "bin/${PROJECT_NAME}" $(pkg-config --libs sdl3); then
            echo "Done"
        else
            exit 1
        fi
    fi
    exit 0
fi
