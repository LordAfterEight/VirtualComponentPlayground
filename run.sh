source project.env

if ./build.sh; then
    echo -e "\n===========================================================================\n"
    ./bin/$PROJECT_NAME
fi
